#include "utils.h"
#include "log.h"
#include "stringview.h"

#include <shv/chainpack/utils.h>
#include <cstring>
#include <regex>
#include <sstream>

#include <unistd.h>

using namespace std;
using namespace shv::chainpack;

namespace shv::core {

namespace {
template<typename Out>
void split(const std::string &s, char delim, Out result)
{
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}
}

int Utils::versionStringToInt(const std::string &version_string)
{
	int ret = 0;
	for(const auto &s : split(version_string, '.')) {
		int i = ::atoi(s.c_str());
		ret = 100 * ret + i;
	}
	return ret;
}

std::string Utils::intToVersionString(int ver)
{
	std::string ret;
	while(ver) {
		int i = ver % 100;
		ver /= 100;
		std::string s = std::to_string(i);
		if(ret.empty())
			ret = s;
		else
			ret = s + '.' + ret;
	}
	return ret;
}

using shv::chainpack::utils::hexNibble;

std::string Utils::toHex(const std::string &bytes)
{
	std::string ret;
	for (char byte : bytes) {
		auto b = static_cast<unsigned char>(byte);
		char h = static_cast<char>(b / 16);
		char l = b % 16;
		ret += hexNibble(h);
		ret += hexNibble(l);
	}
	return ret;
}

std::string Utils::toHex(const std::basic_string<uint8_t> &bytes)
{
	std::string ret;
	for (unsigned char b : bytes) {
		char h = static_cast<char>(b / 16);
		char l = b % 16;
		ret += hexNibble(h);
		ret += hexNibble(l);
	}
	return ret;
}

static inline char unhex_char(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return char(0);
}

std::string Utils::fromHex(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ) {
		auto u = static_cast<unsigned char>(unhex_char(bytes[i++]));
		u = 16 * u;
		if(i < bytes.size())
			u += static_cast<unsigned char>(unhex_char(bytes[i++]));
		ret.push_back(static_cast<char>(u));
	}
	return ret;
}

static void create_key_val(RpcValue &map, const StringViewList &path, const RpcValue &val)
{
	if(path.empty())
		return;
	if(path.size() == 1) {
		map.set(path[static_cast<size_t>(path.length() - 1)].toString(), val);
	}
	else {
		string key = path[0].toString();
		RpcValue mval = map.at(key);
		if(!mval.isMap())
			mval = RpcValue::Map();
		create_key_val(mval, path.mid(1), val);
		map.set(key, mval);
	}
}

RpcValue Utils::foldMap(const chainpack::RpcValue::Map &plain_map, char key_delimiter)
{
	RpcValue ret = RpcValue::Map();
	for(const auto &kv : plain_map) {
		const string &key = kv.first;
		StringViewList lst = StringView(key).split(key_delimiter);
		create_key_val(ret, lst, kv.second);
	}
	return ret;
}

std::string Utils::joinPath(const StringViewList &p)
{
	string ret;
	for(const StringView &path : p) {
		ret = utils::joinPath(ret, path);
	}
	return ret;
}

std::string Utils::joinPath(const StringView &p1, const StringView &p2)
{
	return utils::joinPath(p1, p2);
}

std::string utils::joinPath()
{
	return {};
}

std::string utils::joinPath(const StringView &p1, const StringView &p2)
{
	StringView sv1(p1);
	while(sv1.endsWith('/'))
		sv1 = sv1.mid(0, sv1.size() - 1);
	StringView sv2(p2);
	while(sv2.startsWith('/'))
		sv2 = sv2.mid(1);
	if(sv2.empty())
		return sv1.toString();
	if(sv1.empty())
		return sv2.toString();
	return sv1.toString() + '/' + sv2.toString();
}

std::string Utils::simplifyPath(const std::string &p)
{
	StringViewList ret;
	StringViewList lst = StringView(p).split('/');
	for (size_t i = 0; i < lst.size(); ++i) {
		const StringView &s = lst[i];
		if(s == ".")
			continue;
		if(s == "..") {
			if(!ret.empty())
				ret.pop_back();
			continue;
		}
		ret.push_back(s);
	}
	return ret.join('/');
}

std::vector<char> Utils::readAllFd(int fd)
{
	/// will not work with blockong read !!!
	/// one possible solution for the blocking sockets, pipes, FIFOs, and tty's is
	/// ioctl(fd, FIONREAD, &n);
	static constexpr ssize_t CHUNK_SIZE = 1024;
	std::vector<char> ret;
	while(true) {
		size_t prev_size = ret.size();
		ret.resize(prev_size + CHUNK_SIZE);
		ssize_t n = ::read(fd, ret.data() + prev_size, CHUNK_SIZE);
		if(n < 0) {
			if(errno == EAGAIN) {
				if(prev_size && (prev_size % CHUNK_SIZE)) {
					shvError() << "no data available, returning so far read bytes:" << prev_size << "number of chunks:" << (prev_size/CHUNK_SIZE);
				}
				else {
					/// can happen if previous read returned exactly CHUNK_SIZE
				}
				ret.resize(prev_size);
				return ret;
			}

			shvError() << "error read fd:" << fd << std::strerror(errno);
			return std::vector<char>();
		}
		if(n < CHUNK_SIZE) {
			ret.resize(prev_size + static_cast<size_t>(n));
			break;
		}
#ifdef USE_IOCTL_FIONREAD
		else if(n == CHUNK_SIZE) {
			if(S_ISFIFO(mode) || S_ISSOCK(mode)) {
				if(::ioctl(fd, FIONREAD, &n) < 0) {
					shvError() << "error ioctl(FIONREAD) fd:" << fd << ::strerror(errno);
					return ret;
				}
				if(n == 0)
					return ret;
			}
		}
#endif
	}
	return ret;
}
}
