#ifndef CFGFILE_H
#define CFGFILE_H

#include <string>
#include <vector>

class CfgFile {
	int  str2vec(std::vector<std::string> &lst, const char *src);
  public:
	CfgFile() {}
	~CfgFile() {term();}
	bool init(const char *name, int memSw=0);
	bool init(const std::string &name, int memSw=0) {return init(name.c_str(), memSw);}
	void term();
	int  getVal(const char *tagName);
	int  getVal(const std::string &tagName) {return getVal(tagName.c_str());}
	bool getStr(std::string &dst, const char *tagName);
	bool getStr(std::string &dst, const std::string &tagName) {return getStr(dst, tagName.c_str());}
	bool getStrVec(std::vector<std::string> &lst, const char *tagName);
};


#endif	//CFGFILE_H
