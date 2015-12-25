#include "imio.h"
#include "cMatch.h"

int main(int argc, char* argv[])
{
	img::Img<uint8_t> left, right;
	stereo::CensusMatch cm(0, 0, 0);
	auto disp = cm.match(left, right);
	return 0;
}