#include "utils.h"

//#include <QByteArray>

//#include <unistd.h>

namespace shv {
namespace coreqt {
/*
QByteArray Utils::readAllFd(int fd)
{
	static constexpr ssize_t CHUNK_SIZE = 1024;
	QByteArray ret;
	do {
		size_t prev_size = ret.size();
		ret.resize(ret.size() + CHUNK_SIZE);
		ssize_t n = ::read(fd, ret.data() + prev_size, CHUNK_SIZE);
		if(n < CHUNK_SIZE)
			break;
	} while(true);
	return ret;
}
*/
} // namespace coreqt
} // namespace shv
