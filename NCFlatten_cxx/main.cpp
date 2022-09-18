#define _CRT_SECURE_NO_WARNINGS
#include <omp.h>
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

#define NUM_THREADS 10

const int MAX_VARIABLE = 2;
const int MAX_TIME = 744;
const int MAX_LANT = 721;
const int MAX_LONGT = 1440;

bool new_file_for_once = true;

cxxopts::ParseResult CreateOptions(int argc, char** argv)
{
	cxxopts::Options options("NCFlatten", "A tool for converting grid NC variables to vectors.");

	options.add_options()
		("i,input", "(Required at least this or the \"list\") Input NC files, support single and multiple filename inputs. For example: NCFlatten -i a.nc,b.nc,c.nc or NCFlatten -i a.nc", cxxopts::value<std::vector<std::string>>()->default_value(""))
		//("l,list", "(Required at least this or the \"input\") Input list of NC files. Only support file named by numbers. For example: NCFlatten -i 100.nc,120.nc,140.nc,145.nc means you will choose NO. 100~120 and NO. 140~145 files for the outputs.", cxxopts::value<std::vector<std::string>>()->default_value(""))
		("o,output", "(Optional) Output destination folder (default = current directory). For example: NCFlatten -o ./result or NCFlatten -o C:/result", cxxopts::value<std::string>()->default_value("."))
		("c,coordinate", "(Optional) Specify the range of grid coordinates (lower left and upper right corner coordinates of the grid) for the output vectors. For example: NCFlatten -c 0,0,540,960", cxxopts::value<std::vector<size_t>>()->default_value("0"))
		("v,variable", "(Required) Specify the variables you want to output. For example: NCFlatten -v u10,v10", cxxopts::value<std::vector<std::string>>()->default_value(""))
		//("e,exposure", "Adjust pixel value (default = 1.02607f)", cxxopts::value<float>()->default_value("1.02607"))
		//("g,gamma", "gammmavalue will be used for gamma correction when saving png image(default = 2.2)", cxxopts::value<float>()->default_value("2.2"))
		//("r,resize", "Resize image using `resize_factor`(default = 1.0). 2 = create half size image, 4 = 1/4 image, and so on", cxxopts::value<float>()->default_value("1.0"))
		//("m,mapping", "Mapping Type (default is 0): 0 is Auto, 1 is HorizontalLineCubemap, 2 is VerticalLineCubemap, 3 is Cylindrical", cxxopts::value<int>()->default_value("0"))
		//("f,face", "Specify the facenum (1 or defalut 6) to decide output image numbers", cxxopts::value<int>()->default_value("6"))
		("h,help", "Print usage")
		;

	cxxopts::ParseResult ret = options.parse(argc, argv);

	if (ret.count("input") == 0
		|| ret.count("variable") == 0
		|| ret.count("help"))
	{
		std::cout << options.help() << std::endl;
		system("pause");
		exit(0);
	}

	return ret;
}

void Process(std::string filename, std::string output_path, std::vector<std::string> variables, std::vector<size_t> range)
{
	netCDF::NcFile dataFile;
	bool init_for_once = true;

	std::cout << filename << ":" << std::endl;

	try
	{
		dataFile.open(filename.c_str(), netCDF::NcFile::read);
		std::cout << "reading complete -- variables list: ";

		auto datas = dataFile.getVars();
		std::multimap<std::string, netCDF::NcVar>::iterator it;
		for (it = datas.begin(); it != datas.end(); it++)
			std::cout << (*it).first << " ";
		std::cout << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << "reading error: " << e.what() << std::endl;
		system("pause");
		exit(-1);
	}

	try
	{
		std::cout << "start processing" << std::endl;

		std::vector<netCDF::NcVar> datas;
		std::vector<double> scales;
		std::vector<double> offsets;
		size_t data_time = 0, data_lantitude = 0, data_longtitude = 0;

		for (auto it = variables.begin(); it < variables.end(); it++)
		{
			netCDF::NcVar data = dataFile.getVar(*it);
			if (data.isNull())
				throw std::invalid_argument((*it + " is not available in this file").c_str());
			datas.push_back(data);

			// scale and factor
			netCDF::NcVarAtt att = data.getAtt("scale_factor");
			double dvalue = 0.0; att.getValues(&dvalue);
			scales.push_back(dvalue);

			att = data.getAtt("add_offset");
			dvalue = 0.0; att.getValues(&dvalue);
			offsets.push_back(dvalue);

			// initialize
			if (init_for_once)
			{
				// Assume all the output variable has 3 dimensions: time, lantitude, longtitude.
				//data_time = 6;
				data_time = data.getDim(0).getSize();
				data_lantitude = data.getDim(1).getSize();
				data_longtitude = data.getDim(2).getSize();
				std::cout << "time: " << data_time << " lantitude: " << data_lantitude << " langtitude: " << data_longtitude << std::endl;
				if (range.size() < 4)
				{
					range.clear();
					range.push_back(0); range.push_back(0); range.push_back(data_lantitude - 1); range.push_back(data_longtitude - 1);
				}
				else
				{
					if (range[0] < 0) range[0] = 0;
					if (range[1] < 0) range[1] = 0;
					if (range[2] < range[0]) range[2] = range[0];
					if (range[3] < range[1]) range[3] = range[1];
					if (range[2] >= data_lantitude) range[2] = data_lantitude - 1;
					if (range[3] >= data_longtitude) range[3] = data_longtitude - 1;
				}
				init_for_once = false;
			}
		}

		size_t current_longtitude_count = range[3] - range[1] + 1;
		std::vector<size_t> count = { data_time, 1, current_longtitude_count };

		std::cout << "start writing into txt, please waiting..." << std::endl;

		struct stat info;
		if (stat(output_path.c_str(), &info) != 0)
		{
#if defined(_WIN32)
			int check = _mkdir(output_path.c_str());
#else
			int check = mkdir(output_path.c_str(), 0777);
#endif

			if (check != 0) throw std::invalid_argument(("can not access " + output_path + ", error code=" + std::to_string(check)).c_str());
		}

//#pragma omp parallel for
		for (int i = range[0]; i <= range[2]; i ++)
		{
			//short datas_value[MAX_VARIABLE][MAX_TIME][MAX_LONGT];
			//std::vector<std::vector<std::vector<short>>> datas_value(datas.size(), std::vector<std::vector<short>>(data_time, std::vector<short>(data_longtitude, 0)));
			//std::vector<std::vector<short>> datas_value(data_time, std::vector<short>(data_longtitude, 0));
			std::vector<int*> datas_value(datas.size());
			
			//printf("i = %d, I am Thread %d\n", i, omp_get_thread_num());
			std::vector<size_t> start = { 0, (size_t)i, range[1] };
			for (int v = 0; v < datas.size(); v ++)
			{
				//std::ofstream outfile(output_path + "/test" +  std::to_string(i) + "_" + std::to_string(v) + ".txt", new_file_for_once ? std::ios_base::out : std::ios_base::app);

				datas_value[v] = new int[data_time * current_longtitude_count];
				datas[v].getVar(start, count, datas_value[v]);
				//outfile << "start: " << start[0] << " " << start[1] << " " << start[2] << " count: " << count[0] << " " << count[1] << " " << count[2] << std::endl;
				//for (int x = 0; x < data_time * current_longtitude_count; x ++)
					//printf("%d ", datas_value[v][x]);
				//outfile << std::endl;

				//outfile.close();
			}

#pragma omp parallel for
			for (int j = range[1]; j <= range[3]; j ++)
			{
				//short datas_value[MAX_VARIABLE][MAX_TIME];
				//std::vector<size_t> start = { 0, (size_t)i, (size_t)j };
				//for (int v = 0; v < datas.size(); v++)
				//	datas[v].getVar(start, count, datas_value[v]);

				std::ofstream outfile(output_path + "/" + std::to_string(i) + "_" + std::to_string(j) + ".txt", new_file_for_once ? std::ios_base::out : std::ios_base::app);

				//for (int v = 0; v < datas.size(); v ++)
				//	for (int x = 0; x < data_time * current_longtitude_count; x ++)
				//		outfile << datas_value[v][x] << " ";
				//outfile << std::endl;

				for (int t = 0; t < data_time; t ++)
				{
					for (int v = 0; v < datas.size(); v ++)
						outfile << ((datas_value[v])[t * current_longtitude_count + j - range[1]]) * scales[v] + offsets[v] << " ";
						//outfile << "v: " << v << " t: " << t << " j: " << j << " offset: " << t * current_longtitude_count + j - range[1] << " value: " << (datas_value[v][t * current_longtitude_count + j - range[1]]) << " ";

					outfile << std::endl;
				}

				outfile.close();
			}

			for (int v = 0; v < datas.size(); v++)
				delete datas_value[v];

			std::cout << "task #" << i - range[0] << " complete" << std::endl;
		}


		std::cout << "processing complete" << std::endl;
		new_file_for_once = false;
		dataFile.close();

	}
	catch (const std::exception& e)
	{
		std::cout << "processing error: " << e.what() << std::endl;
		dataFile.close();
	}

}

int main(int argc, char** argv)
{
	cxxopts::ParseResult result = CreateOptions(argc, (char** )argv);

	std::vector<std::string> input_single_files = result["input"].as<std::vector<std::string>>();
	//std::vector<std::string> input_list_files = result["list"].as<std::vector<std::string>>();
	std::string output_path = result["output"].as<std::string>();
	std::vector<std::string> variables = result["variable"].as<std::vector<std::string>>();

	std::vector<size_t> range = result["coordinate"].as<std::vector<size_t>>();


	std::string filename;
	for (auto it = input_single_files.begin(); it != input_single_files.end(); it++)
	{
		filename = (*it).c_str();
		Process(filename, output_path, variables, range);
		std::cout << std::endl;
	}

	system("pause");
	return 0;
}