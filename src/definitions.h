// Copyright © 2018-2019 Tokarev Artem Alekseevich. All rights reserved.
// Contacts: <tokarev28.art@gmail.com>
// Licensed under the MIT License.

#pragma once

namespace tva {
struct Vec;
typedef Vec Point;
} // namespace tva

namespace pmg {
class Polycrystal;
class Crystallite;
class Tetr;
class Facet;
class Edge;
class Vertex;

namespace front {
namespace plane {
class Edge;
class Vertex;
} // namespace plane

namespace surface {
class Facet;
class Edge;
} // namespace surface
} // namespace front

namespace shell {
class Facet;
class Edge;
class Vertex;
} // namespace shell
} // namespace pmg
