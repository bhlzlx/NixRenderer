#pragma once
struct BufferAllocation {
	uint64_t	buffer;
	size_t		offset;
	size_t		size;
	uint16_t	allocationId;
	uint8_t*	raw;
	BufferAllocation()
		: buffer(0)
		, offset(0)
		, size(0)
		, allocationId(-1)
		, raw(nullptr) {
	}
};
