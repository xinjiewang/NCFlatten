#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <netcdfcpp.h>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#endif
#include <fstream>  

#include "cxxopts.h"
using namespace std;

cxxopts::ParseResult CreateOptions(int argc, char** argv)
{
	cxxopts::Options options("NCFlatten", "A tool for converting grid NC variables to vectors.");

	options.add_options()
		("i,input", "(Required at least this or the \"list\") Input NC files, support single and multiple filename inputs. For example: NCFlatten -i a.nc,b.nc,c.nc or NCFlatten -i a.nc", cxxopts::value<vector<string>>())
		("l,list", "(Required at least this or the \"input\") Input list of NC files. Only support file named by numbers. For example: NCFlatten -i 100.nc,120.nc,140.nc,145.nc means you will choose NO. 100~120 and NO. 140~145 files for the outputs.", cxxopts::value<vector<string>>()->default_value(""))
		("o,output", "(Optional) Output destination folder (default = current directory). For example: NCFlatten -o ./result or NCFlatten -o C:/result", cxxopts::value<string>()->default_value("."))
		("c,coordinate", "(Optional) Specify the range of grid coordinates (lower left and upper right corner coordinates of the grid) for the output vectors. For example: NCFlatten -c 0,0,540,960", cxxopts::value<vector<int>>()->default_value("-1"))
		("v,variable", "(Required) Specify the variables you want to output. For example: NCFlatten -v u10,v10", cxxopts::value<vector<string>>())
		//("e,exposure", "Adjust pixel value (default = 1.02607f)", cxxopts::value<float>()->default_value("1.02607"))
		//("g,gamma", "gammmavalue will be used for gamma correction when saving png image(default = 2.2)", cxxopts::value<float>()->default_value("2.2"))
		//("r,resize", "Resize image using `resize_factor`(default = 1.0). 2 = create half size image, 4 = 1/4 image, and so on", cxxopts::value<float>()->default_value("1.0"))
		//("m,mapping", "Mapping Type (default is 0): 0 is Auto, 1 is HorizontalLineCubemap, 2 is VerticalLineCubemap, 3 is Cylindrical", cxxopts::value<int>()->default_value("0"))
		//("f,face", "Specify the facenum (1 or defalut 6) to decide output image numbers", cxxopts::value<int>()->default_value("6"))
		("h,help", "Print usage")
		;

	cxxopts::ParseResult ret = options.parse(argc, argv);

	if ((ret.count("input") == 0 && ret.count("list") == 0)
		|| ret.count("variable") == 0
		|| ret.count("help"))
	{
		cout << options.help() << endl;
		exit(0);
	}

	return ret;
}

void Process(string filename, string output_path, vector<string> variables, vector<int> range)
{
	netCDF::NcFile dataFile;

	bool getRangeForOnce = false;

	try
	{
		dataFile.open(filename.c_str(), netCDF::NcFile::read);
		cout << "reading complete" << endl;

		auto datas = dataFile.getVars();
		multimap<string, netCDF::NcVar>::iterator it;
		for (it = datas.begin(); it != datas.end(); it++)
			cout << (*it).first << endl;
	}
	catch (const exception& e)
	{
		cout << filename << " reading error: " << e.what() << endl;
		exit(-1);
	}

	try
	{
		for (auto it = variables.begin(); it < variables.end(); it++)
		{
			netCDF::NcVar data = dataFile.getVar(*it);
			if (data.isNull())
				throw invalid_argument((*it + " is not available in file " + filename).c_str());

			// Process
			// Assume all the output variable has 3 dimensions: time, lantitude, longtitude.
			int time = data.getDim(0).getSize(), lantitude = data.getDim(1).getSize(), longtitude = data.getDim(2).getSize();
			cout << time << " " << lantitude << " " << longtitude << endl;

			if (getRangeForOnce)
			{
				if (range.size() < 4)
				{
					range.clear();
					range.push_back(0);range.push_back(0);range.push_back(lantitude - 1);range.push_back(longtitude - 1);
				}
				else
				{
					if (range[0] < 0) range[0] = 0;
					if (range[1] < 0) range[1] = 0;
					if (range[2] >= lantitude) range[2] = lantitude - 1;
					if (range[3] >= longtitude) range[3] = longtitude - 1;
				}
				getRangeForOnce = false;
			}

			string output_filename;
			for (int i = range[0]; i <= range[2]; i ++)
				for (int j = range[1]; j <= range[3]; j++)
				{

					struct stat info;
					if (stat(output_path.c_str(), &info) != 0)
					{
						#if defined(_WIN32)
						int check = _mkdir(output_path.c_str());
						#else
						int check = mkdir(output_path.c_str(), 0777);
						#endif

						if (check != 0) throw invalid_argument(("Can not access " + output_path + ", error code=" + to_string(check)).c_str());
					}
					std::ofstream outfile(output_path + "/" + to_string(i) + "_" + to_string(j) + ".txt");

					outfile << "my text here!" << std::endl;

					outfile.close();
				}
		}

		dataFile.close();
	}
	catch (const exception& e)
	{
		cout << "processing error: " << e.what() << endl;
		dataFile.close();
		exit(-1);
	}

}

int main(int argc, char** argv)
{
	cxxopts::ParseResult result = CreateOptions(argc, (char** )argv);

	vector<string> input_single_files = result["input"].as<vector<string>>();
	vector<string> input_list_files = result["list"].as<vector<string>>();
	string output_path = result["output"].as<string>();
	vector<string> variables = result["variable"].as<vector<string>>();

	vector<int> range = result["coordinate"].as<vector<int>>();


	string filename;
	for (auto it = input_single_files.begin(); it != input_single_files.end(); it++)
	{
		filename = (*it).c_str();
		Process(filename, output_path, variables, range);
	}

	return 0;
}