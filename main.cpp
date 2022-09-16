#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <netcdfcpp.h>
#include <string>

#include "cxxopts.h"

cxxopts::ParseResult CreateOptions(int argc, char** argv)
{
	cxxopts::Options options("NCFlatten", "A tool for converting grid NC variables to vectors.");

	options.add_options()
		("i,input", "(Required) Input NC files, support single and multiple filename inputs. For example: NCFlatten -i a.nc,b.nc,c.nc or NCFlatten -i a.nc", cxxopts::value<std::vector<std::string>>())
		("o,output", "Output destination folder (default = current directory). For example: NCFlatten -o ./result or NCFlatten -o C:/result", cxxopts::value<std::string>()->default_value("."))
		("c,coord", "Specify the range of grid coordinates (lower left and upper right corner coordinates of the grid) for the output vectors. For example: NCFlatten -c 0,0,540,960", cxxopts::value<std::vector<int>>()->default_value("0"))
		//("e,exposure", "Adjust pixel value (default = 1.02607f)", cxxopts::value<float>()->default_value("1.02607"))
		//("g,gamma", "gammmavalue will be used for gamma correction when saving png image(default = 2.2)", cxxopts::value<float>()->default_value("2.2"))
		//("r,resize", "Resize image using `resize_factor`(default = 1.0). 2 = create half size image, 4 = 1/4 image, and so on", cxxopts::value<float>()->default_value("1.0"))
		//("m,mapping", "Mapping Type (default is 0): 0 is Auto, 1 is HorizontalLineCubemap, 2 is VerticalLineCubemap, 3 is Cylindrical", cxxopts::value<int>()->default_value("0"))
		//("f,face", "Specify the facenum (1 or defalut 6) to decide output image numbers", cxxopts::value<int>()->default_value("6"))
		("h,help", "Print usage")
		;


	cxxopts::ParseResult ret = options.parse(argc, argv);

	if (ret.count("input") == 0
		|| ret.count("output") == 0
		|| ret.count("help"))
	{
		std::cout << options.help() << std::endl;
		exit(0);
	}

	return ret;

}

int main(int argc, char** argv)
{
	cxxopts::ParseResult result = CreateOptions(argc, (char** )argv);

	std::vector<std::string> input_files = result["input"].as< std::vector<std::string>>();
	std::string output_path = result["output"].as<std::string>();

	std::vector<int> range = result["coord"].as<std::vector<int>>();
	bool allGrid = false;
	if (range.size() < 4) allGrid = true;

	netCDF::NcFile dataFile;
	try
	{
		dataFile.open(input_files[0].c_str(), netCDF::NcFile::read);
		std::cout << "reading complete" << std::endl;

		auto datas = dataFile.getVars();
		std::multimap<std::string, netCDF::NcVar>::iterator it;
		for (it = datas.begin(); it != datas.end(); it++)
		    std::cout << (*it).first << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << input_files[0] << " reading error: " << e.what() << std::endl;
		exit(-1);
	}

	try
	{
		netCDF::NcVar udata = dataFile.getVar("u10");
		if (udata.isNull()) 
			throw new std::exception(("u10 is not available in file " + input_files[0]).c_str());

	}
	catch (const std::exception& e)
	{
		std::cout << "processing error: " << e.what() << std::endl;
	}

	return 0;
}