import ix

def mat2arr(mat):
    result = []
    for i in range(0,16):
        result.append(mat.get_item(i%4, i/4))
    return result


def scan(path):
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

            result.append(item)

            return result

        elif mod.get_class_info().get_name() == "ModuleSceneObjectTree":
            base = mod.get_base_objects()

            subscene = {}

            objs = []
            for i in range(0, base.get_count()):
                obj = scan(base[i].get_object().get_full_name())
                objs.append(obj)
            subscene["objects"] = objs;

            inst = mod.get_instances()
            mats = ix.api.GMathMatrix4x4dArray()
            mod.get_instance_matrices(mats)

            if inst.get_count() > 0:
                assert inst.get_count() == mats.get_count()

                instances = []
                for i in range(0, inst.get_count()):
                    ii = {}
                    ii["id"] = inst[i]
                    ii["transform"] = mat2arr(mats[i])

                    instances.append(ii)
                subscene["instances"] = instances

            result.append(subscene)

            return result

    # recursive scan of contexts
    elif obj.is_context():
        ctx = obj.to_context()
        items = ix.api.OfItemVector()
        ctx.get_items(items)
        for o in items:
            items = scan(o.get_full_name())
            if items:
                result = result + items
        return result

    else:
        return None

res = scan('project://scene')
print res