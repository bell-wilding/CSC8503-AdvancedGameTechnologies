#pragma once
#include "NavigationMap.h"
#include <string>
#include <random>

namespace NCL {
	namespace CSC8503 {
		struct GridNode {
			GridNode* parent;

			GridNode* connected[4];
			int		  costs[4];

			Vector3		position;

			float f;
			float g;

			bool closed = false;
			bool open	= false;

			int type;

			GridNode() {
				for (int i = 0; i < 4; ++i) {
					connected[i] = nullptr;
					costs[i] = 0;
				}
				f = 0;
				g = 0;
				type = 0;
				parent = nullptr;
			}
			~GridNode() {	}
		};

		struct CompareMapNode {
			bool operator()(GridNode* const& n1, GridNode* const& n2) {
				return n1->f > n2->f;
			}
		};

		class NavigationGrid : public NavigationMap	{
		public:
			NavigationGrid();
			NavigationGrid(const std::string&filename);
			~NavigationGrid();

			bool FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) override;

			int GetGridWidth() const {
				return gridWidth;
			}

			int GetGridHeight() const {
				return gridHeight;
			}

			int GetNodeSize() const {
				return nodeSize;
			}

			Vector3 GetRandomValidPosition() const;
				
		protected:
			bool		NodeInList(GridNode* n, std::vector<GridNode*>& list) const;
			GridNode*	RemoveBestNode(std::vector<GridNode*>& list) const;
			float		Heuristic(GridNode* hNode, GridNode* endNode) const;
			int nodeSize;
			int gridWidth;
			int gridHeight;

			std::vector<Vector3> validPositions;

			GridNode* allNodes;
		};
	}
}

