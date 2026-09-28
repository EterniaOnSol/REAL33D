#include "Real33DItemUsePolicy.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv)
{
	const auto ids = Real33D::ReadMultiUseTypeIds(
		"TypeID = 3272\r\nFlags = {Take, MultiUse, Weapon} # comment\n"
		"TypeID = 2854\nName = \"MultiUse # backpack\"\nFlags = {Container,Take}\n"
		"TypeID = 2853\nFlags = {NotMultiUse,Container}\n"
		"TypeID = 3000\nFlags = {MultiUse}\n"
		"TypeID = invalid\nFlags = {MultiUse}\n"
		"TypeID = 65535\nFlags = {MultiUse}\n");
	assert(ids.size() == 2 && ids.count(3272) && ids.count(3000));
	assert(!ids.count(2854) && !ids.count(2853) && !ids.count(65535));
	assert(Real33D::ReadMultiUseTypeIds("Flags = {MultiUse}\n").empty());
	if (argc == 2)
	{
		std::ifstream file(argv[1], std::ios::binary);
		assert(file.good());
		const std::string table((std::istreambuf_iterator<char>(file)), {});
		const auto liveIds = Real33D::ReadMultiUseTypeIds(table);
		assert(liveIds.count(3272) && !liveIds.count(2854) && !liveIds.count(2853));
		assert(liveIds.count(3148) && liveIds.count(3149) && !liveIds.count(3147));
		std::cout << "runtime MultiUse metadata PASS: " << liveIds.size() << " types\n";
	}
	std::cout << "item use policy PASS\n";
}
