//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/types/row_operations/row_aggregate.cpp
//
//
//===----------------------------------------------------------------------===//
#include "duckdb/common/row_operations/row_operations.hpp"

#include "duckdb/common/types/row_layout.hpp"
#include "duckdb/catalog/catalog_entry/aggregate_function_catalog_entry.hpp"

namespace duckdb {

void RowOperations::InitializeStates(RowLayout &layout, Vector &addresses, const SelectionVector &sel, idx_t count) {
	if (count == 0) {
		return;
	}
	auto pointers = FlatVector::GetData<data_ptr_t>(addresses);
	auto &offsets = layout.GetOffsets();
	auto aggr_idx = layout.ColumnCount();

	for (auto &aggr : layout.GetAggregates()) {
		for (idx_t i = 0; i < count; ++i) {
			auto row_idx = sel.get_index(i);
			auto row = pointers[row_idx];
			aggr.function.initialize(row + offsets[aggr_idx]);
		}
		++aggr_idx;
	}
}

void RowOperations::DestroyStates(RowLayout &layout, Vector &addresses, idx_t count) {
	if (count == 0) {
		return;
	}
	//	Move to the first aggregate state
	VectorOperations::AddInPlace(addresses, layout.GetAggrOffset(), count);
	for (auto &aggr : layout.GetAggregates()) {
		if (aggr.function.destructor) {
			aggr.function.destructor(addresses, count);
		}
		// move to the next aggregate state
		VectorOperations::AddInPlace(addresses, aggr.payload_size, count);
	}
}

} // namespace duckdb
