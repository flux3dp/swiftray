#ifndef SHAPECOLLECTION_H
#define SHAPECOLLECTION_H

#include <shape/shape.hpp>
using namespace std;
typedef QList<shared_ptr<Shape>> ShapeCollection;
typedef shared_ptr<Shape> ShapePtr;