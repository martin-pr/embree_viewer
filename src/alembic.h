#pragma once

#include <boost/filesystem/path.hpp>

#include "scene.h"

Scene loadAlembic(boost::filesystem::path path);
