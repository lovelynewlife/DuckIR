//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/optimizer/join_order/plan_enumerator.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/unordered_set.hpp"
#include "duckdb/optimizer/join_order/join_relation.hpp"
#include "duckdb/optimizer/join_order/cardinality_estimator.hpp"
#include "duckdb/optimizer/join_order/query_graph.hpp"
#include "duckdb/optimizer/join_order/join_node.hpp"
#include "duckdb/optimizer/join_order/cost_model.hpp"
#include "duckdb/parser/expression_map.hpp"
#include "duckdb/planner/logical_operator.hpp"
#include "duckdb/planner/logical_operator_visitor.hpp"

#include <functional>

namespace duckdb {

class PlanEnumerator {
public:
	explicit PlanEnumerator(QueryGraphManager query_graph_manager, CostModel cost_model) : query_graph_manager(query_graph_manager), cost_model(cost_model),
	      full_plan_found(false),
	      must_update_full_plan(false) {}

	//! Perform the join order solving
	JoinNode* SolveJoinOrder();

private:

	//! The total amount of join pairs that have been considered
	idx_t pairs = 0;
	//! The set of edges used in the join optimizer
	QueryGraphManager query_graph_manager;
	//! Cost model to evaluate cost of joins
	CostModel cost_model;
	//! A map to store the optimal join plan found for a specific JoinRelationSet*
	unordered_map<JoinRelationSet *, unique_ptr<JoinNode>> plans;

	bool full_plan_found;
	bool must_update_full_plan;
	unordered_set<std::string> join_nodes_in_full_plan;

	//! Extract the bindings referred to by an Expression
	bool ExtractBindings(Expression &expression, unordered_set<idx_t> &bindings);

	//! Get column bindings from a filter
	void GetColumnBinding(Expression &expression, ColumnBinding &binding);

	//! Traverse the query tree to find (1) base relations, (2) existing join conditions and (3) filters that can be
	//! rewritten into joins. Returns true if there are joins in the tree that can be reordered, false otherwise.
	bool ExtractJoinRelations(LogicalOperator &input_op, vector<reference<LogicalOperator>> &filter_operators,
	                          optional_ptr<LogicalOperator> parent = nullptr);

	//! Emit a pair as a potential join candidate. Returns the best plan found for the (left, right) connection (either
	//! the newly created plan, or an existing plan)
	JoinNode &EmitPair(JoinRelationSet &left, JoinRelationSet &right, const vector<reference<NeighborInfo>> &info);
	//! Tries to emit a potential join candidate pair. Returns false if too many pairs have already been emitted,
	//! cancelling the dynamic programming step.
	bool TryEmitPair(JoinRelationSet &left, JoinRelationSet &right, const vector<reference<NeighborInfo>> &info);

	bool EnumerateCmpRecursive(JoinRelationSet &left, JoinRelationSet &right, unordered_set<idx_t> &exclusion_set);
	//! Emit a relation set node
	bool EmitCSG(JoinRelationSet &node);
	//! Enumerate the possible connected subgraphs that can be joined together in the join graph
	bool EnumerateCSGRecursive(JoinRelationSet &node, unordered_set<idx_t> &exclusion_set);
	//! Rewrite a logical query plan given the join plan
	unique_ptr<LogicalOperator> RewritePlan(unique_ptr<LogicalOperator> plan, JoinNode &node);
	//! Generate cross product edges inside the side
	void GenerateCrossProducts();

	//! Solve the join order exactly using dynamic programming. Returns true if it was completed successfully (i.e. did
	//! not time-out)
	bool SolveJoinOrderExactly();
	//! Solve the join order approximately using a greedy algorithm
	void SolveJoinOrderApproximately();

	void UpdateDPTree(JoinNode &new_plan);

	void UpdateJoinNodesInFullPlan(JoinNode &node);
	bool NodeInFullPlan(JoinNode &node);

};

} // namespace duckdb
