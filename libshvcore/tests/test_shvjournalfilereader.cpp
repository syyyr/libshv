#include <shv/core/utils/shvjournalfilereader.h>
#include <necrolog.h>

#include <chrono>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::core::utils;
using namespace shv::core;
using namespace std;

namespace {
const std::string FILES_DIR = DEF_FILES_DIR;
}

template <class TimeT  = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock>
struct measureDuration
{
	template<class F, class ...Args>
	static auto duration(F&& func, Args&&... args)
	{
		auto start = ClockT::now();
		std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
		return std::chrono::duration_cast<TimeT>(ClockT::now() - start);
	}
};

DOCTEST_TEST_CASE("ShvJournalFileReader speed")
{
	auto start = chrono::steady_clock::now();
	auto log_file_name = FILES_DIR + '/' + "dirtylog";
	try {
		ShvJournalFileReader rd(log_file_name);
		int line_cnt = 0;
		while(rd.next()) {
			if(const auto &e = rd.entry(); e.isValid())
				++line_cnt;
		}
		auto msec = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
		nInfo() << line_cnt << "lines of dirty log read in" << msec.count() << "msec.";
		REQUIRE(line_cnt > 0);	}
	catch (const shv::core::Exception &exc) {
		nError() << "Cannot open file:" << log_file_name << ", consider checking validity of symlink.";
	}
}
