#pragma once
#include "../../Common/Vector2.h"
#include "../CSC8503Common/CollisionDetection.h"
#include "Debug.h"
#include <list>
#include <functional>

namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class Octree;

		template<class T>
		struct OctreeEntry {
			Vector3 pos;
			Vector3 size;
			T object;

			OctreeEntry(T obj, Vector3 pos, Vector3 size) {
				object		= obj;
				this->pos	= pos;
				this->size	= size;
			}
		};

		template<class T>
		class OctreeNode {
		public:
			typedef std::function<void(std::list<OctreeEntry<T>>&)> OctTreeFunc;
		protected:
			friend class Octree<T>;

			OctreeNode() {}

			OctreeNode(Vector3 pos, Vector3 size) {
				children		= nullptr;
				this->position	= pos;
				this->size		= size;
			}

			~OctreeNode() {
				delete[] children;
			}

			void Insert(T& object, const Vector3& objectPos, const Vector3& objectSize, int depthLeft, int maxSize) {
				if (!CollisionDetection::AABBTest(objectPos, 
					position, objectSize, size)) {
					return;
				}
				if (children) {
					for (int i = 0; i < 8; ++i) {
						children[i].Insert(object, objectPos, objectSize, depthLeft - 1, maxSize);
					}
				}
				else {
					contents.push_back(OctreeEntry<T>(object, objectPos, objectSize));
					if ((int)contents.size() > maxSize && depthLeft > 0) {
						if (!children) {
							Split();
							for (const auto& i : contents) {
								for (int j = 0; j < 8; ++j) {
									auto entry = i;
									children[j].Insert(entry.object, entry.pos, entry.size, depthLeft - 1, maxSize);
								}
							}
							contents.clear();
						}
					}
				}
			}

			void GetCollidingNodes(T& object, const Vector3& objectPos, const Vector3& objectSize, std::list<OctreeEntry<T>>& collidingNodes) {
				if (!CollisionDetection::AABBTest(objectPos, position, objectSize, size)) {
					return;
				}
				if (children) {
					for (int i = 0; i < 8; ++i) {
						children[i].GetCollidingNodes(object, objectPos, objectSize, collidingNodes);
					}
				}
				else {
					for (auto x : contents) {
						collidingNodes.push_back(x);
					}
				}
			}

			void Split() {
				Vector3 halfSize = size / 2.0f;
				children = new OctreeNode<T>[8];
				children[0] = OctreeNode<T>(position + Vector3(-halfSize.x, halfSize.y, halfSize.z), halfSize);
				children[1] = OctreeNode<T>(position + Vector3(halfSize.x, halfSize.y, halfSize.z), halfSize);
				children[2] = OctreeNode<T>(position + Vector3(-halfSize.x, -halfSize.y, halfSize.z), halfSize);
				children[3] = OctreeNode<T>(position + Vector3(halfSize.x, -halfSize.y, halfSize.z), halfSize);
				children[4] = OctreeNode<T>(position + Vector3(-halfSize.x, halfSize.y, -halfSize.z), halfSize);
				children[5] = OctreeNode<T>(position + Vector3(halfSize.x, halfSize.y, -halfSize.z), halfSize);
				children[6] = OctreeNode<T>(position + Vector3(-halfSize.x, -halfSize.y, -halfSize.z), halfSize);
				children[7] = OctreeNode<T>(position + Vector3(halfSize.x, -halfSize.y, -halfSize.z), halfSize);
			}

			void DebugDraw() {

			}

			void OperateOnContents(OctTreeFunc& func) {
				if (children) {
					for (int i = 0; i < 8; ++i) {
						children[i].OperateOnContents(func);
					}
				}
				else {
					if (!contents.empty()) {
						func(contents);
					}
				}
			}

		protected:
			std::list< OctreeEntry<T> >	contents;

			Vector3 position;
			Vector3 size;

			OctreeNode<T>* children;
		};
	}
}


namespace NCL {
	using namespace NCL::Maths;
	namespace CSC8503 {
		template<class T>
		class Octree
		{
		public:
			Octree(Vector3 size, int maxDepth = 6, int maxSize = 5){
				root = OctreeNode<T>(Vector3(), size);
				this->maxDepth	= maxDepth;
				this->maxSize	= maxSize;
			}
			~Octree() {
			}

			void Insert(T object, const Vector3& pos, const Vector3& size) {
				root.Insert(object, pos, size, maxDepth, maxSize);
			}

			void GetCollidingNodes(T object, const Vector3& pos, const Vector3& size, std::list<OctreeEntry<T>>& collidingNodes) {
				root.GetCollidingNodes(object, pos, size, collidingNodes);
			}

			void DebugDraw() {
				root.DebugDraw();
			}

			void OperateOnContents(typename OctreeNode<T>::OctTreeFunc  func) {
				root.OperateOnContents(func);
			}

		protected:
			OctreeNode<T> root;
			int maxDepth;
			int maxSize;
		};
	}
}