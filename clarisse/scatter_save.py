import ix
import os
import json
import struct
from sets import Set

# set to True to only leave "leaf" filenames (not full paths) in the file
STRIP_FILENAMES = False
# maximum number of instanced objects inlined in a file
MAX_INLINE_INSTANCES = 10

def mat2arr(mat):
    result = []
    for i in range(0,16):
        result.append(mat.get_item(i%4, i/4))
    return result


def scan(path, target_dir, saved_scatterers):
    result = []

    obj = ix.application.get_factory().get_item(path)

    # single object handling
    if obj.is_object():
        obj = obj.to_object()
        mod = obj.get_module()
        # if it has a filename and an instance matrix, its an object that can be exported
        if getattr(mod, 'get_object_matrix', None) and obj.attribute_exists('filename'):
            item = {}
            item["path"] = obj.get_attribute('filename').get_string()
            item["transform"] = mat2arr(mod.get_object_matrix())

            if STRIP_FILENAMES:
                item["path"] = os.path.basename(item["path"])

            result.append(item)

        elif mod.get_class_info().get_name() == "ModuleSceneObjectTree":
            base = mod.get_base_objects()

            subscene = {}

            objs = []
            for i in range(0, base.get_count()):
                ii = scan(base[i].get_object().get_full_name(), target_dir, saved_scatterers)
                objs.append(ii)
            subscene["objects"] = objs;
            subscene["transform"] = mat2arr(mod.get_object_matrix())

            inst = mod.get_instances()
            mats = ix.api.GMathMatrix4x4dArray()
            mod.get_instance_matrices(mats)

            if inst.get_count() > 0:
                assert inst.get_count() == mats.get_count()

                # inlined scattering, if there are not too many instances for a json file
                if inst.get_count() < MAX_INLINE_INSTANCES:
                    instances = []
                    for i in range(0, inst.get_count()):
                        ii = {}
                        ii["id"] = inst[i]
                        ii["transform"] = mat2arr(mats[i])

                        instances.append(ii)
                    subscene["instances"] = instances
                # write out a binary dump when there are too many instances to be easily represented in a json
                else:
                    filename = obj.get_full_name().replace('/', '__').replace(':', '__') + '.bin'

                    if not filename in saved_scatterers:
                        saved_scatterers.add(filename)

                        instance_file = open(target_dir + '/' + filename, "wb")
                        for i in range(0, inst.get_count()):
                            instance_file.write(struct.pack('I', inst[i]))
                            instance_file.write(struct.pack('16f', *mat2arr(mats[i])))

                        subscene["instance_file"] = filename

            result.append(subscene)

    # recursive scan of contexts
    elif obj.is_context():
        ctx = obj.to_context()
        items = ix.api.OfItemVector()
        ctx.get_items(items)
        for o in items:
            items = scan(o.get_full_name(), target_dir, saved_scatterers)
            if items:
                result = result + items

    return result

def writeData(path, target_dir):
    saved_scatterers = Set()
    result = scan(path, target_dir, saved_scatterers)

    with open(target_dir + '/scene.json', 'w') as outfile:
        json.dump(result, outfile, indent=4)

writeData('project://scene', '/home/martin/Dropbox/Coding/embree_viewer/data/Grass')
