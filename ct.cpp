
/*

Copyright (C) 2023, Steve Jin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


/*
 * DSXCUT
 *
 * ***********
 * Description:
 * ***********
 * This utility takes DSX file as input. The DSX file is generated from DS Designer through "export" command
 * This utility search component (job, routine and etc) and write to an individual file.
 * File name of the output file is same as the component name, with ".dsx" as file name extention.
 * Output file always in a subfolder according its folder/hireachy in DS Designer.
 *
 * Optionaly, list the component's name, type and hierachy without write to file. By option -l
 * Optionaly, no output to screen. By option -q
 * Opitnaly, specify the folder that contain all the output dsx files. By option -o
 *
 *
 * ------
 * Filter
 * ------
 *  Filter of component is applied in processing. Only following type of object are processed.
 *   job(parallel and sequence),
 *   parameterSet
 *   routine
 *
 *   These are needed at compile time. Design time components can be ignored.
 *
 * ---------------------------------------
 *  Object types to be includeed in future
 * ---------------------------------------
 *  Some types can be added in future: (add entries to comp_types structure)
 *    transform (DS Designer advanced design)
 *    shareContainer (DS Designer advanced design)
 *
 *    matchSpecification (analys/profile)
 *
 *    dataElement (admin level, engine feature)
 *    machineProfile (admin level, engine feature)
 *    MNSRules (admin level, engine feature)
 *    stageType (admin level, engine feature)
 *    standardizationRules (admin level, engine feature)
 *    tableDefinition (design phase only, not needed in compile time, EXCLUDED EXPLICATLY)
 *
 *
 *
 * ***********
 * Execution:
 * ***********
 *
 * 	--------------------------------------------
 * 	C:\cygwin64\home\jin\py\cutter>dsxcut
 * 	Error: input file must be provided!

 * 	Usage:
 * 	    a <.dsx file> [-lqhoc] [-o outdir]

 *
 * 	Allowed options:
 * 	  -f [ --file ] arg        REQUIED. Input .dsx file
 * 	  -l [ --list ]            List components only (No write)
 * 	  -q [ --quiet ]           Suppress output(quiet)
 * 	  -o [ --outdir ] arg (=.) Output dir (Default: current working directory)
 * 	  -h [ --help ]            This help msg
 *
 *
 * 	C:\cygwin64\home\jin\py\cutter>
 * 	--------------------------------------------
 *
 * Return Code:
 * 	0: 	success
 * 	non-0:	error
 *
 *
 * ************
 * Distribution
 * ************
 * Only dsxcut.exe (windows) or dsxcut (Linux) is needed for distribution.
 * No environment runtime environmnet is needed.
 *
 * See "Compatibility" session for compatibility troubleshooting/workaround.
 *
 *
 */


/* Build from source
 *
 * **********
 * with Windows(mingW under cygwin)
 * **********
 * In cygwin environment
 * 	o Install libmingW, gcc 4.8.5(stdc++11) or higher and libboost(dev) 1.66.0+.
 * 	o Build:
 *
 * 		--------------------------------------------
 * 		jin@host ~/py/cutter
 * 		$ clear;time cat ct.cpp | /usr/bin/i686-w64-mingw32-g++ -O2 --std=c++11 -g -xc++ - -L/usr/i686-w64-mingw32/sys-root/mingw/lib  -l:libboost_system.a -l:libboost_filesystem.a -l:libboost_regex.a -l:libboost_program_options.a -l:libstdc++.a -l:libpthread.a -static-libgcc -static-libstdc++ -static -o dsxcut && time ./dsxcut.exe *.dsx
 *
 * 		--------------------------------------------
 *
 * 	Optionally, install boost source (required for Linux only, if no root/admin permission is obtained),
 * 		build and link with main program as practice for Linux environment building process.
 *
 *
 *
 * **********
 * with Linux
 * **********
 *   Minimum installation: gcc 4.8.5
 *   Boost:
 * 	o Download Boost source package v1.66.0 from officail site at:
 * 		https://boostorg.jfrog.io/artifactory/main/release/1.66.0/source/boost_1_66_0.tar.bz2
 * 	o Extract to seperate folder (with at least 600MB disk space)
 * 	o Configure it with command 
 * 		./bootstrap.sh
 * 	o Build component with:
 * 		./b2 stage --with-regex --with-filesystem --with-system address-model=64 -d+2 -j6
 * 		The built library files are in folder ./stage .
 *
 *   Util:
 * 	o Build, use same command line in mingW, with proper -L path replaced with
 * 		boost built in first step (under boost/.../stage folder) from
 *
 * *************
 * Compatibility:
 * *************
 *
 * 	Windows. The link parameter used is to have a "self-contained" stand-alone 
 * 		binary that can be run in any Windows 10 or up, (no run-time 
 * 		library/environment is needed).
 *
 *  	Linux: Under Linux, there are glibc version dependency, since gcc is chosen. 
 *  		Though all Linux has glibc runtime environment installed by default. 
 *  		Not all versions provides 100% backward compabiblity to the glibc 
 *  			version used for linking.  
 *  		The workaround is to build with "oldest" system you can find and 
 *  			distribute the executable to "newer" systems.
 *  		This is based on glibc backward compatibility. For details, 
 *  			check https://abi-laboratory.pro/?view=timeline&l=glibc
 *
 */

#define BOOST
#define DEBUG 0

#if DEBUG==1
#define D(x) x
#else
#define D(x)

#endif


#include <string>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>	// stat()

#include <fstream>
#include <streambuf>
#include <cstdlib>
//#include <filesystem>	// create_directoryies(), cwd()


//#include <conio>	// getchar()

#include <unistd.h>
//#include <stacktrace>

#ifdef BOOST
//#include <boost/system>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#else
#include <regex>		// require c++11 up
#endif


using namespace std;
namespace po = boost::program_options;

bool b_oList=false, b_oQuiet=false, b_oStdin=false;

bool validate16(const std::string &s) {
#ifdef BOOST
	static const boost::regex e("(\\d{4}[- ]){3}\\d{4}");
	return boost::regex_match(s, e);
#else
	static const std::regex e("(\\d{4}[- ]){3}\\d{4}");
	return std::regex_match(s, e);
#endif
}




template<typename ... Args>
std::string string_format(const std::string & format, Args ... args) {
	int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) +1;
	if (size_s < 0) {throw std::runtime_error("Error in string_format()");}
	auto size = static_cast <size_t>(size_s);
	
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size -1);

}

#define DSXNEWLINE "\x0d\x0a"
typedef struct {
	string type_short;
	string type;
	int entity_level;	// if 2, with 1st level seperator as "^   DSRECORD$".
	int name_level;
	int cate_level;
	string name_prefix;
	string cate_prefix;
}comp_t;

enum {
	F_TYPE,
	F_BSTART,
	F_BEND,
	F_LEVEL,
	F_NSTART,
	F_NEND,
	F_NQUOTE
};

std::vector<comp_t> comp_types {
	    {"HD", "HEADER" ,  1, 0, 0, "Identifier", "Category"},
	    {"JOB", "DSJOB" , 1, 1, 2, "Identifier", "Category"},
	    // {"DT", "DSDATATYPES" , 2, 2, 2, "Identifier", "Category"},	// not needed at runtime
	    {"PS", "DSPARAMETERSETS" , 2,2, 2, "Identifier", "Category"},
	    // {"TD", "DSTABLEDEFS" , 2, 2, 2, "Identifier",  "Category"},	// not needed at runtime
	    {"RT", "DSROUTINES" , 2, 2, 2, "Identifier",  "Category"},

	    // Assumed ... , to be confirmed/tested !!!!
	    {"TR", "DSTRANSFORMES" , 2, 2, 2, "Identifier",  "Category"},
	    {"ST", "DSSTAGETYPES" , 2, 2, 2, "Identifier",  "Category"},
	    {"SC", "DSSHAREDCONTAINER" , 1, 1, 2, "Identifier",  "Category"},

	    // Binaries of component is explicitly excluded/ignored !!! As assumption that execution
	    // enviroments configureation generally are not the same, therefore, compilation is requireed
	    // after code promotion/imported.
};



bool withCRLF(string str, size_t pos) {
	if (0 == str.compare(pos, strlen(DSXNEWLINE), DSXNEWLINE))
		return true;
	return false;
}

bool skipCRLF(string str, size_t & pos) {
	if (withCRLF(str, pos)) {
		pos += strlen(DSXNEWLINE);
		return true;
	}
	else {
		return false;
	}
}
class Component {

	private:
	string mBody;
	string mType;
	string mName;
	string mCate;

	//int ofs_curr;

	public:
	Component() {}
	Component(std::string body, std::string type, std::string name, std::string cate)
       	{
		setBody(body);
		setType(type);
		setName(name);
		setCate(cate);
	}
	static inline std::string getDirSeptChar() {
#ifdef _WIN32
		//std::cout << "It is WIN32 FS" << std::endl;
		return "\\";
#else
		//std::cout << "It is Unix/Linux FS " << std::endl;
		return "/";
#endif
	}
	void clear() {
		mBody.clear();
		mType.clear();
		mName.clear();
		mCate.clear();
	}

	std::string getPath() {
		boost::regex expr("\\\\\\\\");
		std::string fmt(Component::getDirSeptChar());
		return boost::regex_replace(mCate, expr, fmt);

	}

	void setBody(std::string body) { mBody = body; }
	void setType(std::string type) { mType = type; }
	void setName(std::string name) { mName = name; }
	void setCate(std::string cate) { mCate = cate; }
	std::string getBody() {return mBody;}
	std::string getType() {return mType;}
	std::string getName() {return mName;}
	std::string getCate() {return mCate;}

	int getLevelByType(std::string type) {
		int hit = -1;
		int i=0;
		for(comp_t elem: comp_types) {
			if (elem.type == type) {hit = 1; break;}
			i++;
		}

		if (hit == 0) {
			//std::cout << "getLevelByType(): hit=0, return -1" <<std::endl;
			return -1;
		}

		return comp_types[i].entity_level;
	}

	std::string getAttr(int level, std::string str){
		std::string spa = ((level==1)? "   ": "      " ) + str + " \"([^\"]+)\"" DSXNEWLINE;
		boost::regex expr { spa };
		boost::smatch result;
		if (boost::regex_search(mBody, result, expr)) {
			return result[1];
		}
		return NULL;
	}
		
	std::string pack2() {
		//std::cout <<" pack2" <<std::endl;
		int L = getLevelByType(mType);
		//std::cout << "pack2(): L= " <<L<<std::endl;
		if (L < 1) {
			//std::cout << "hit L <1 "<< std::endl;
			return NULL;
		}
		string x= (L == 1)? mBody : ("   BEGIN DSRECORD"  DSXNEWLINE + mBody + "   END DSRECORD"  DSXNEWLINE);

		return mBody;
	}
	std::string pack1() {
		//std::cout <<" pack1" <<std::endl;
		int L = getLevelByType(mType);
		string sType = getType();
		//std::cout << "pack1(): type= " << sType << std::endl;
		if (L == 1) return mBody;
		if (L == 2) {
			return ("BEGIN "+ sType+ DSXNEWLINE + pack2() + "END "+ sType + DSXNEWLINE);
		}


		return NULL;
	}

	/*
	bool parseObj(string body, int &ofs); // parse and populate fields.

	int search(string body, int &ofs);	// find first qualify Component ofs
	*/
}; //class  Component


class Collection {
	private:
	std::string mBody0;
	std::string mFilename;

	std::string mHeader;
	vector<Component*> mComponents;

	int mOfs_curr;

	public:
	Collection(const char * osFile) {
		mBody0.clear();
		mFilename = osFile;
	}
	Collection() {
		mBody0.clear();
	}

	vector<Component*> getComponents() {
		return mComponents;
	}
	string getHeader() {
		return mHeader;
	}
	int count () {
		return mComponents.size();
	}
	std::string pack0(Component *c) {
		if (DEBUG) std::cout <<" pack0: "<< c->getType() <<", "<< c->getName() <<std::endl;
		return mHeader + c->pack1() ;
	}
	// read file, all-in-one dsx file
	bool readFile(){
		try {
			std::ifstream ifs(mFilename, std::ifstream::binary | std::ifstream::in);
			ifs.seekg(0, std::ios::end);
			mBody0.reserve(ifs.tellg());
			ifs.seekg(0, std::ios::beg);
			mBody0.assign((std::istreambuf_iterator<char>(ifs)),
				       std::istreambuf_iterator<char>());
			ifs.close();
		} catch(const std::exception &e) {
			return false;
		}

		return true;
	}
	// write file, one component per file
	bool writeFile(std::string path, std::string filename, std::string content) {
		struct stat f_stat;

		if (stat(path.c_str(), &f_stat) != 0 &&
			 !boost::filesystem::create_directories(path)) {
			std::string msg = string_format("Error: Can't create directory %s", path);
			std::cout << " Failed." << msg << std::endl;
				return false;
		}
		std::string fullPath = path + Component::getDirSeptChar() + filename + ".dsx";
		
		std::ofstream ofs(fullPath, std::ifstream::binary | std::ifstream::out);
		ofs << content;
		ofs.close();
		return true;
	}


	void parse() {

		boost::regex expr0 {"(?! )BEGIN ([A-Z]*)" DSXNEWLINE ".*?[^ ]END \\1" DSXNEWLINE  };
		boost::regex_token_iterator<std::string::iterator> it0{mBody0.begin(),mBody0.end(), expr0};
		boost::regex_token_iterator<std::string::iterator> end0;

		int cnt = 0;
		while (it0 != end0) {
			std::string body1 = *it0++;

			int t0,t1;
			t0 = body1.find(" ");
			t1 = body1.find("\r\n");
			std::string type = body1.substr(t0+1, t1+1 - t0 -2);
	
			int hit = -1;

			int i=0;
			for (comp_t elem : comp_types) {
				if (elem.type == type) {hit = 1; break;}
				i++;
			}
			if (hit == -1) continue;

			if (DEBUG) std::cout << "type=[" << type << "]" ;
			if (comp_types[i].entity_level == 1) {

				Component *c = new Component();
				c->setBody(body1);
				c->setType(type);
				if (comp_types[i].name_level> 0) {
					c->setName(c->getAttr(comp_types[i].name_level, comp_types[i].name_prefix));
					if (DEBUG) { std::cout << " name = [" << c->getName()  << "]"; }
				}
				if (comp_types[i].cate_level> 0) {
					c->setCate(c->getAttr(comp_types[i].cate_level, comp_types[i].cate_prefix));
					if (DEBUG) { std::cout << " cate = [" << c->getCate()  << "]"; }
				}
				if (DEBUG) std::cout << endl;

				if (c->getType() == "HEADER") mHeader = c->getBody();
				else mComponents.push_back(c);
			} else if (comp_types[i].entity_level == 2) {

//				std::cout << "hit 2 <<<<<<<<<<<<" <<std::endl;
//				std::cout << body1;
//				std::cout << "hit 2 <<<<<<<<<<<<" <<std::endl;

				boost::regex expr1 {"   BEGIN (DSRECORD" DSXNEWLINE ").*?   END \\1" };		// works!
				boost::regex_token_iterator<std::string::iterator> it1{body1.begin(),body1.end(), expr1};
				boost::regex_token_iterator<std::string::iterator> end1;


				int ii=0;
				while (it1 != end1) {
					ii++;
					std::string body2 = *it1++;

//					std::cout << "body2 <<<<<<<<<<<<" << "#" << ii <<std::endl;
//					std::cout << body2 ;
//					std::cout << "body2 <<<<<<<<<<<<" << "#" << ii <<std::endl;

					if (DEBUG) { std::cout << endl << ""; }

					Component *c = new Component();
					c->setBody(body2);
					c->setType(type);
					if (comp_types[i].name_level> 0) {
						c->setName(c->getAttr(comp_types[i].name_level, comp_types[i].name_prefix));
						if (DEBUG) { std::cout << "\tname = [" << c->getName()  << "]"; }
					}
					if (comp_types[i].cate_level> 0) {
						c->setCate(c->getAttr(comp_types[i].cate_level, comp_types[i].cate_prefix));
						if (DEBUG) { std::cout << " cate = [" << c->getCate()  << "]"; }
					}
					if (DEBUG) std::cout << endl;

					mComponents.push_back(c);
				}
			} // level 1 vs 2.


			/*
			//std::cout << body1 << std::endl;
			boost::regex expr1_i {"[^ ]   Identifier \"(\\w+)\"" DSXNEWLINE};
			boost::regex expr1_c {"[^ ]      Category \"([^\"]+)\"" DSXNEWLINE};
			boost::smatch result_i, result_c;
	
			if (boost::regex_search(body1, result_c, expr1_c)) {
				std::cout << "====cate\t" << result_c[1] <<std::endl;
			}
			if (boost::regex_search(body1, result_i, expr1_i)) {
				std::cout << "====iden\t" << result_i[1] <<std::endl;
			}
			std::cout << "size=\t" << body1.size() << std::endl ;
			std::cout << std::endl;
			*/
			
		}// while
	}// parse()

		
		

	int dumpToFiles(string path);	// write to files
	int merge(Component x);
	int composeToFile(string Path, string filename);	// write to one file.



}; //class Collection

	


po::options_description desc("Allowed options");

void usage(int argc, char ** argv) {
	std::cout << std::endl;
	std::cout << "Usage: " << std::endl<< "    " << argv[0] << " <.dsx file> [-lqhoc] [-o outdir] " << std::endl << std::endl;
	std::cout << desc <<std::endl;
	std::cout << "    Built with gcc" << __VERSION__ << ", at "<< __TIME__ <<" "<< __DATE__ << std::endl;

}

bool po_validate(int argc, char ** argv, po::variables_map vm) {
	// file
	struct stat f_stat;
	if (!vm.count("file")) {
		std::cout << "Error: Input file need to be provided" << std::endl;
		usage(argc, argv);
		exit(1);
	}
	auto file = vm["file"].as<std::string>().c_str();
	if (stat(file, &f_stat) != 0) {
		std::cout << "Error: Input file " << file << " doesn't exist" << std::endl;
		usage(argc, argv);
		exit(1);
	}
	std::string xx;
	if (f_stat.st_size >= xx.max_size()) {
		std::cout << "Error: Input file too big! ";
		std::cout << "This is a "<< 8*sizeof(size_t)<< "-bit program.";
		exit(1);
	}
	
	if (!(S_ISREG (f_stat.st_mode) &&
		f_stat.st_mode &S_IRUSR)) {
		std::string msg = string_format("Error: Source file \"%s\" not accessable", file);
		std::cout << msg << std::endl;
		usage(argc, argv);
		exit(1);
	}

	// target
	if (vm["list"].as<bool>() == 0) {
		struct stat f_stat;
		const char * outdir = vm["outdir"].as<std::string>().c_str();
		if (DEBUG) std::cout << "Checking target dir [" << outdir << "].." << std::endl;
		if (strcmp(outdir, ".")  != 0) {
			if (stat(outdir, &f_stat) != 0) {
				if (!b_oQuiet) std::cout << "Info: target directory \"" << outdir << "\" not exist. Create it ..." ;

				boost::filesystem::path _path = outdir ;
				if (!boost::filesystem::create_directories(_path)) {
					std::string msg = string_format("Error: Can't create directory %s", outdir);
					std::cout << " Failed." << msg << std::endl;
					usage(argc, argv);
					exit(1);
				}
				std::cout << " Success." << std::endl;
			}
		}
	}


	return true;

}


bool fs_env(int argc, char ** argv) {

	//std::cout << "artc=" << argc << std::endl;
	
	if (argc == 1) {
		std::cout << "Error, not enough argument" << std::endl;
		//usage(argc, argv);
		std::cout << "temp usage()" << std::endl;
		exit(1);
	}
	// source file .dsx
	if (argc >= 2) {
		// check file existance
		struct stat f_stat;

		if (stat(argv[1], &f_stat) != 0) {
			std::cout << "Error: Input file " << argv[1] << " doesn't exist" << std::endl;
			//usage(argc, argv);
		std::cout << "temp usage()" << std::endl;
			exit(1);
		} else if(!(S_ISREG (f_stat.st_mode) &&
				f_stat.st_mode &S_IRUSR )) {
			//msg
			//
			std::string msg = string_format("Error: Source file \"%s\" not accessable", argv[1]);
			std::cout << msg << std::endl;
			//usage(argc, argv);
		std::cout << "temp usage()" << std::endl;
			exit(1);
		}

	}
	// check target
	if (argc >=3) {
		struct stat f_stat;

		if (stat(argv[2], &f_stat) != 0) {
			std::cout << "Info: target directory \"" << argv[2] << "\" not exist. Creat it ..." ;

			char cmd[300];
			sprintf(cmd, "mkdir -p %s", argv[2]);
			const int dir_err = system(cmd);
			if (-1 == dir_err) {
				std::string msg = string_format("Error: Can't create directory %s", argv[2]);
				std::cout << " Failed." << msg << std::endl;
				//usage(argc, argv);
		std::cout << "temp usage()" << std::endl;
				exit(1);
			}
			std::cout << " Success." << std::endl;
		}

	}





	


	return true;
} // fs_env()

/*
void e_handler(int sig) {
	void *array[10];
	size_t size;

	//get void *'s for all entries on the stack
	size = backtrace(array,10);
	
	//print out all the rames to stderr
	frpintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbol_fd(array, size, STDERR_FILENO);
	exit(1);
}
*/

void handle_eptr(std::exception_ptr eptr) {
	try {
		if (eptr) std::rethrow_exception(eptr);
	} catch(const std::exception &e) {
		std::cout<< "Caught exception \"" <<e.what() <<"\"\n";
	}
}

int main(int argc, char ** argv) {

	std::exception_ptr eptr;

	try {
	
		// asserts
		//std::cout << "max string size: " << str.max_size() << std::endl;
	
		///////////// po //////////////////
	
		po::variables_map vm;
	
		try {
			desc.add_options()
				("file,f", po::value<std::string>(), "REQUIED. Input .dsx file")
				("list,l", po::bool_switch(&b_oList), "List components only (No write)")
				("quiet,q", po::bool_switch(&b_oQuiet), "Suppress output(quiet)")
		
				("outdir,o", po::value<std::string>()->composing()->default_value("."), "Output dir (Default: current working directory)")
				//("stdin,c", po::bool_switch(&b_oStdin), "DSX file content from stdin")
				("help,h", "This help msg")
				;
		
			po::positional_options_description p;
			p.add("file", -1);
		
	
			po::store(po::command_line_parser(argc, argv).
				options(desc).positional(p).allow_unregistered().run(), vm);
	
			po::notify(vm);
		
			// list all args
			D(for (const auto &it: vm) {
				auto& value = it.second.value();;
				std::cout << it.first
					<< ':';
				if (auto v = boost::any_cast<int> (&value))
					std::cout << *v;
				else if (auto v = boost::any_cast<std::string> (&value))
					std::cout << *v;
				else if (auto v = boost::any_cast<bool> (&value))
					std::cout << *v;
				else
					std::cout << "error";
		
				std::cout
					<< std::endl;
			}
			)
		
			// validations
			if (DEBUG) std::cout << "=========validation==============" << std::endl;
			if (!vm.count("file")  && b_oStdin == false ) {
				std::cout << "Error: input file must be provided!" << std::endl;
				usage(argc,argv);
				return 1;
			}
			//if (!b_oQuiet) std::cout << "file = " << vm["file"].as<std::string>() << std::endl;
		
			po_validate(argc, argv, vm);
		
		
		
		} catch (const std::exception &e) {
			std::cout << "ERROR: "<< std::endl << "ERROR: " << e.what() << " argument error" << std::endl;
			std::cout << "ERROR:" << std::endl;
			usage(argc, argv);
			//throw("argument error");
			return -1;
		}
	
		
		///////////// po //////////////////
	
	
	
		// validate in/out fs environment.
		//fs_env(argc, argv);
	
		// target dir
		std::string cwd = boost::filesystem::current_path().string();
		if (DEBUG) std::cout << "CWD=[" << cwd <<"]"<< std::endl;
		//std::string targetDIR = (argc == 3) ? argv[2]: cwd;
		std::string targetDIR = vm["outdir"].as<std::string>();
		if (targetDIR == ".") targetDIR = cwd;
		else targetDIR = cwd + Component::getDirSeptChar()+ targetDIR;
		
	
		//std::cout << "b_oQuiet="<<b_oQuiet << endl;
		//std::cout << "b_oList="<<b_oList << endl;
		if (!b_oQuiet) {
			std::cout << "DSX file: \t"<< vm["file"].as<std::string>() << std::endl;
			std::cout << "output DIR:\t"<< targetDIR << std::endl;
			if (b_oList) std::cout << "**LIST ONLY**" << std::endl;

			std::cout << std::endl;
		}
	
	
	
		// get .dsx file
		//Collection coll = Collection(argv[1]);
		Collection coll = Collection(vm["file"].as<std::string>().c_str());
		if (! coll.readFile()) { //catch exceptions ...
			if (!b_oQuiet) {
				std::cout << "Error in read file [" << vm["file"].as<std::string>() << "]. Quit" << std::endl;
			}
			return -1;
		}
	
		// parse
		coll.parse();


		int N = coll.count();
		if (N == 0) {
			std::cerr << "** no component found in file**"<< std::endl;
			return 0;
		} else if (coll.getHeader().size() == 0) {
			cerr << "**no header**" << endl;
			throw("file has no header");
		}

		int n=1;
		// iterate the items.
		for(Component* c : coll.getComponents()) {
			std::string fileContent = coll.pack0(c);
			std::string FullPath;
			std::string rPath = c->getPath();
			FullPath = targetDIR + rPath;
	
			//std::cout <<"rPATH=["<< rPath <<"]" <<std::endl;
			if (!b_oQuiet) {
				string _name = c->getName();
				string _path = c->getPath();
				string _type = c->getType();
				string _n = string_format("[%03d/%03d]", n, N);
	
				std::cout << _n <<
					string_format("  %-18s" , _type.c_str()) <<
					string_format("  %-28s" , _path.c_str()) <<
					string_format("  %-28s", _name.c_str()) << std::endl;
			}
	
			// write individual file
			if (!b_oList &&  !coll.writeFile(FullPath, c->getName(), fileContent)) {
				std::cout << "Error: fail to write file [" <<FullPath << Component::getDirSeptChar() << c->getName() << "]" << std::endl;
				
			}
	
			//std::cout << "writeFile '" << c->getName() <<"' to [" << FullPath << "] ... " << ((ret.size() > 0)? "OK":"Failed") << std::endl;
	
			
			n++;
	
		}

	} catch( char * msg) {
		std::cerr << msg << std::endl;
		return -1;
	} catch( string str) {
		std::cerr << str << std::endl;
		return -1;
	} catch(const std::exception &e) {
		eptr = std::current_exception(); // capture
		std::cerr << "There is an excpetion: ";
		std::cerr << e.what() << std::endl;
	}
	handle_eptr(eptr);
	//usage(argc, argv);

	//std::cout << "--end" << std::endl;


	//char c;
	//std::cin >>c;
	//std::cin.get();
	//

	return 0;
} // main()
