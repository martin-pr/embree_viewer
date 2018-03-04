#pragma once

#include <boost/filesystem/path.hpp>

#include "json.hpp"

#include "scene.h"

Scene loadMesh(const boost::filesystem::path& p);
Scene parseScene(const nlohmann::json& source, const boost::filesystem::path& scene_root);
