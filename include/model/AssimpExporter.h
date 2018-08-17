#pragma once

#include <string>

namespace model
{

class AssimpExporter
{
public:
	static void Export(const std::string& src_path, const std::string& dst_path);

}; // AssimpExporter

}