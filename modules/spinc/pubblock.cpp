#include <iostream>
#include <string>

#include "spinc.hpp"

PUBBlock::PUBBlock(std::string code)
{
	std::cout << "PUB block: " << code << '\n';
}


PUBBlockFactory::PUBBlockFactory()
{
	printf("PUB block registered!\n");
}

Block* PUBBlockFactory::newBlock(std::string code) const
{
	return new PUBBlock(code);
}

std::string PUBBlockFactory::getName() const
{
	return "pub";
}

