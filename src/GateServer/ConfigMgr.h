#pragma once
#include "const.h"

//瑙ｆ瀽閰嶇疆鏂囦欢config.ini
//SectionInfo -> key:Port  value:8080
class SectionInfo {
public:
	SectionInfo() = default;
	~SectionInfo() = default;
	//閲嶈浇
	SectionInfo(const SectionInfo& src);
	SectionInfo& operator=(const SectionInfo& src);
	std::string operator[](const std::string& key);
	std::map<std::string, std::string> _section_datas;
};

//ConfigMgr -> key:[GateServer]  value:SectionInfo
class ConfigMgr//鍗曚緥妯″紡
{
public:
	static ConfigMgr& getInstance() {
		static ConfigMgr _instance;
		return _instance;
	}
	//閲嶈浇
	SectionInfo operator[](const std::string& section);

private:
	ConfigMgr();
	ConfigMgr(const ConfigMgr&) = delete;
	ConfigMgr& operator=(const ConfigMgr&) = delete;
	std::map<std::string, SectionInfo> _config_map;
};

