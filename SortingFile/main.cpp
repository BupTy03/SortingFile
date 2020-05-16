#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <execution>
#include <filesystem>


constexpr auto maxBuffersCount = 50;
constexpr auto maxBufferSize = 10000 + 1;

constexpr auto inputFilename = "input.txt";
constexpr auto outputFilename = "output.txt";


template<typename Compare>
void merge_buffer_to_file(
	std::array<std::string, maxBuffersCount>& buffers,
	std::size_t buffersUsed,
	std::vector<std::fstream::pos_type>& positions,
	std::fstream& out,
	Compare comp
)
{
	if (std::empty(positions))
	{
		for (std::size_t i = 0; i < buffersUsed; ++i)
		{
			auto& buffer = buffers[i];
			buffer.push_back('\n');
			out.write(buffer.c_str(), buffer.size());
			positions.emplace_back(out.tellp());
		}
		return;
	}

	std::size_t first1 = 0;
	std::size_t last1 = std::size(buffers);

	std::size_t first2 = 0;
	std::size_t last2 = std::size(positions);

	while (first1 != last1) 
	{
		if (first2 == last2)
		{
			for (; first1 != last1; ++first1)
			{
				auto& buf = buffers[first1];
				buf.push_back('\n');
				out.write(buf.c_str(), buf.size());
				positions.emplace_back(out.tellp());
			}

			return;
		}

		if (comp(positions[first2], buffers[first1]))
		{
			++first2;
		}
		else 
		{
			const auto old_pos = positions[first2];
			out.clear();
			out.seekp(old_pos);

			auto& buf = buffers[first1];

			buf.push_back('\n');
			out.write(buf.c_str(), std::size(buf));

			auto oldIt = std::next(positions.emplace(std::cbegin(positions) + first2, old_pos));
			*oldIt = out.tellp();

			++first1;
		}
	}
}


void sort_with_buffer()
{
	std::ifstream inputFile(inputFilename);
	std::fstream outputFile(outputFilename, std::ios_base::in | std::ios_base::out);

	std::vector<std::fstream::pos_type> positions;
	std::array<std::string, maxBuffersCount> buffers;
	for (auto& buffer : buffers)
		buffer.reserve(maxBufferSize);

	std::string compareBuffer;
	compareBuffer.reserve(maxBufferSize);

	while (inputFile)
	{
		std::size_t buffersUsed = 0;
		for (; inputFile && buffersUsed < maxBuffersCount; ++buffersUsed)
			std::getline(inputFile, buffers[buffersUsed]);

		while (std::empty(buffers[buffersUsed - 1]))
			--buffersUsed;

		std::sort(std::begin(buffers), std::begin(buffers) + buffersUsed);

		merge_buffer_to_file(buffers, buffersUsed, positions, outputFile,
			[&outputFile, &compareBuffer](std::fstream::pos_type pos, const std::string& str) -> bool
			{
				outputFile.clear();
				outputFile.seekg(pos);

				std::getline(outputFile, compareBuffer);
				const bool result = compareBuffer < str;
				compareBuffer.resize(0);
				return result;
			});

		for (std::size_t i = 0; i < buffersUsed; ++i)
			buffers[i].resize(0);
	}

	outputFile.close();
}

void quick_sort_file()
{
	std::ifstream inputFile(inputFilename);
	std::ofstream outputFile(outputFilename);

	std::vector<std::ifstream::pos_type> positions;
	std::istreambuf_iterator<char> inputFileBegin(inputFile);
	std::istreambuf_iterator<char> inputFileEnd;

	inputFile.unsetf(std::ios::skipws);
	positions.emplace_back(inputFile.tellg());
	for (auto it = inputFileBegin; it != inputFileEnd;)
	{
		if (*it == '\n')
		{
			++it;
			positions.emplace_back(inputFile.tellg());
		}
		else
		{
			++it;
		}
	}

	std::sort(positions.begin(), positions.end(), [&inputFile](auto lhs, auto rhs)
		{
			inputFile.clear();
			inputFile.seekg(lhs);

			std::string lhsStr;
			std::getline(inputFile, lhsStr);

			inputFile.clear();
			inputFile.seekg(rhs);

			std::string rhsStr;
			std::getline(inputFile, rhsStr);

			return lhsStr < rhsStr;
		});

	for (auto pos : positions)
	{
		inputFile.clear();
		inputFile.seekg(pos);

		std::string s;
		std::getline(inputFile, s);

		if (s.empty())
			continue;

		outputFile << s << '\n';
	}

	outputFile.close();
}


std::int64_t exec_time(std::function<void()> func)
{
	auto beginTime = std::chrono::steady_clock::now();
	func();
	auto endTime = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count();
}

void print_time(std::function<void()> func, std::function<void()> prepare)
{
	constexpr auto testCount = 1;
	std::int64_t timeMs = std::numeric_limits<std::int64_t>::max();
	for (std::size_t i = 0; i < testCount; ++i)
	{
		prepare();
		timeMs = std::min(timeMs, exec_time(func));
	}

	std::cout << "Time: " << timeMs << "ms." << std::endl;
}

int main()
{
	auto truncate_file = []() { std::filesystem::remove(outputFilename); };

	std::cout << "sort_with_buffer" << std::endl;
	print_time(sort_with_buffer, truncate_file);
	std::cout << std::endl;

	std::cout << "quick_sort_file" << std::endl;
	print_time(quick_sort_file, truncate_file);
	return 0;
}