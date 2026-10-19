#pragma once
#include <cstdint>
#include <vector>


class BranchPredictor
{
public:
	BranchPredictor(std::uint32_t rows, std::uint8_t defaultState);
	std::uint8_t predictTaken(std::uint32_t branchPC);
	void update(std::uint32_t branchPC, std::uint8_t taken);
	std::uint8_t lookup(std::uint32_t branchPC, std::uint8_t& history) const;

private:
	struct Entry
	{
		std::uint32_t pc = 0;
		std::uint8_t history = 0;
		std::uint8_t valid = 0;
	};

	std::uint32_t indexPC(std::uint32_t branchPC) const;
	BranchPredictor::Entry& findOrAlloc(std::uint32_t branchPC);

	std::uint32_t m_rows;
	std::uint8_t m_defaultState;
	std::vector<Entry> m_table;
};