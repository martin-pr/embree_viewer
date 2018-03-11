# Embree Viewer

<img align="right" src="preview.gif">

Embree viewer is a simple implementation of a **progressive renderer**, based on Intel's [Embree raytracing kernels](https://embree.github.io/). Its UI is written in [SDL2](https://www.libsdl.org/), and it supports [Alembic](http://www.alembic.io/) and OBJ model file formats, with a simple JSON file to describe a scene.

## Use case

Embree viewer is intended as a simple example implementation of a progressive renderer with Embree, one step above the simple examples Embree ships with, but significantly simpler than [Ospray](https://github.com/ospray/ospray) and similar.

Its main purpose is to demonstrate the **performance** of Embree raytracing kernels, and the **scalability** of its instancing system, in a simple **real-time multithread framework** that can load common model files. It is not intended as a full raytracer - it does not have any support for materials, textures, normals, lighting or sampling.

## Clarisse scripts

Isotropix Clarisse is a professional lighting tool, heavily focusing on efficient object instancing. It is available as a professional package, or as a Personal Learning Edition.

For the purposes of experimentation with Embree, the `clarisse` directory contains a number of PLE scene files, with a set of scripts to export their scattering setup to a set of JSON and binary files compatible with Embree viewer.

# Building

Embree viewer was developed on a standard installation of Debian Linux; it has not been tested on MS Windows (any contribution in that respect would be appreciated).

## Dependencies

Non-standard dependencies, which have to be compiled on the target Linux machine, and installed into a location available to CMake:

- [Embree 3.0+](https://github.com/embree/embree/releases)
- [Alembic 1.7+](https://github.com/alembic/alembic/releases)

Standard dependencies, available in Debian (or other Linux distro):

- Boost (tested against 1.62.0)
- OpenEXR and IlmBase (tested against 2.2.0)
- SDL2 (tested against 2.0.5)
- TBB (tested against 4.3)

## Compilation

The repository contains a standard set of CMake build files:

```
mkdir build
cmake ..
make -j
```

# Usage

## Command line options

```
Allowed options:
  --help                produce help message
  --mesh arg            load mesh file (.abc, .obj)
  --scene arg           load a scene file (.json)
```

## File formats

At the moment, there are 3 input file types - `.abc`, `.obj` and `.json`.

### Scene file format

Scene file is a very simple JSON-based file format, describing the input files and scattering information.

The root of the scene is a json `list`, enumerating the elements of the scene. Each element can either be a _subscene_ or an _object_.

Each _object_ is represented as a simple dictionary, with a filesystem `path` (absolute or relative) to an `.obj` or a `.abc` mesh file, and a 4x4 matrix `transform` represented as an array.

A _scene_ is a dictionary containing an array of `objects` (each either an object, or another sub-scene), a `transform` acting as a parent for all objects and either a set of `instances` in an array of structs with `id` and `transform`, or an `instance_file` link to a binary file containing the instancing information.

### Binary instances file format

The _instancing file_ is a simple binary file, containing records of 17x32bit data records:
* first 4 bytes represent a 32-bit unsigned integer, referencing which of the `objects` records should be used for this particular instance
* 16 4-byte records after that represent a `transformation matrix` of each instance (composed of 32-bit floats)

## Example files

Embree viewer comes with a small number of example files in the data directory (each directory includes a LICENSE file for the files it contains):

### Virtual Emily mesh

<img align="right" src="emily.png" width="100">

Virtual Emily project's main mesh, demonstrating how to load a single model file.

```
/embree_viewer --mesh data/Emily/Emily_2_1_Alembic_Scene.abc
```

### A Grass Scatterer

<img align="right" src="grass.png" width="100">

A simple scattering scene, using assets exported from Blender and Clarisse, showing the performance and possible complexity of the scene.

```
/embree_viewer --scene data/Grass/scene.json
```

## License and contributions

This demo is licensed under **MIT license**, and as such you can use this code for both commercial and noncommercial purposes.

Any **contributions are welcome**, in terms of ideas, improvements, bugfixes or additional example files.
