#pragma once
#include "mesh-base.h"
#include "../simplex.h"
#include "conversion.h"

namespace spt {

template <
    template <typename... Args> typename Pointer, 
    std::size_t N, std::size_t Dim, typename Real>
struct mesh_base<Pointer, spt::simplex<N, Dim, Real>> {
    using real_type = Real;
    using vertex_type = spt::vertex<Dim, real_type>;
    // ...
    using facet_type = spt::simplex<N - 1, Dim, real_type>;
    using elem_type = spt::simplex<N, Dim, real_type>;

    std::vector<Pointer<vertex_type>> vertices;
    // ...
    std::vector<Pointer<facet_type>> facets;
    std::vector<Pointer<elem_type>> elements;

    std::enable_if_t<std::is_same_v<std::unique_ptr<void>, Pointer<void>>, spt::raw_mesh<elem_type>> get() { 
        return spt::to_raw_mesh<elem_type>(*this); 
    }

    std::enable_if_t<!std::is_same_v<std::unique_ptr<void>, Pointer<void>>, mesh_base>& operator=(const mesh_base& other) {
        vertices = other.vertices;
        // ...
        facets = other.facets;
        elements = other.elements;
        return *this;
    }
    mesh_base& operator=(mesh_base&& other) noexcept {
        vertices = std::move(other.vertices);
        // ...
        facets = std::move(other.facets);
        elements = std::move(other.elements);
        return *this;
    }

    mesh_base() {}
    template <typename MeshBase>
    mesh_base(MeshBase&& other) {
        *this = std::forward<MeshBase>(other);
    }
};


template <
    template <typename... Args> typename Pointer, 
    std::size_t N, std::size_t Dim, typename Real>
struct mesh_base<Pointer, spt::simplex_v<N, Dim, Real>> {
    using real_type = Real;
    using vertex_type = spt::vertex<Dim, real_type>;
    using elem_type = spt::simplex_v<N, Dim, real_type>;

    std::vector<Pointer<vertex_type>> vertices;
    std::vector<Pointer<elem_type>> elements;

    template <typename = std::enable_if_t<std::is_same_v<std::unique_ptr<void>, Pointer<void>>>>
    auto get() { return spt::to_raw_mesh(*this); }

    template <typename = std::enable_if_t<!std::is_same_v<std::unique_ptr<void>, Pointer<void>>>>
    mesh_base& operator=(const mesh_base& other) {
        vertices = other.vertices;
        elements = other.elements;
        return *this;
    }
    mesh_base& operator=(mesh_base&& other) noexcept {
        vertices = std::move(other.vertices);
        elements = std::move(other.elements);
        return *this;
    }

    mesh_base() {}
    template <typename MeshBase>
    mesh_base(MeshBase&& other) {
        *this = std::forward<MeshBase>(other);
    }
};

} // namespace spt
