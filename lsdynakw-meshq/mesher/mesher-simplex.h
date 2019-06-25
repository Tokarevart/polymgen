#pragma once
#include "mesher-base.h"


namespace pmg {

template <typename Polytope, template <typename... Args> typename Pointer>
class mesher<spt::single<Polytope>, spt::simplex, Pointer> {
    using vertex_type = spt::polytope<0, Polytope::dim, typename Polytope::real_type>;
    using edge_type = spt::polytope<1, Polytope::dim, typename Polytope::real_type>;
    using facet_type = spt::polytope<Polytope::n - 1, Polytope::dim, typename Polytope::real_type>;

public:
    using polytope_type = Polytope;
    using shell_type = spt::mesh_base<Pointer, facet_type>;
    using shell_mesh_unit_type = spt::simplex<Polytope::n - 1, Polytope::dim, typename Polytope::real_type>;
    using shell_mesh_type = spt::mesh_base<Pointer, shell_mesh_unit_type, spt::multi>;
    using mesh_unit_type = spt::simplex<Polytope::n, Polytope::dim, typename Polytope::real_type>;
    using mesh_type = spt::mesh_base<Pointer, mesh_unit_type>;
    using real_type = typename polytope_type::real_type;

    void run(real_type preferred_length,
             const genparams<polytope_type>& gen_params = genparams<polytope_type>());

    mesher(const shell_type& shell, const shell_mesh_type& mesh) {
        m_shell = shell;
        m_shell_mesh = mesh;
    }
    mesher(shell_type&& shell, shell_mesh_type&& mesh) noexcept {
        m_shell = std::move(shell);
        m_shell_mesh = std::move(mesh);
    }
    mesher(const shell_type& shell) {
        m_shell = shell;
        mesher<spt::multi<facet_type>, spt::simplex> sh_mesher(shell);
        // mesh the shell...
        m_shell_mesh = std::move(/*mesh*/);
    }
    mesher(shell_type&& shell) noexcept {
        m_shell = std::move(shell);
        mesher<spt::multi<facet_type>, spt::simplex> sh_mesher(shell);
        // mesh the shell...
        m_shell_mesh = std::move(/*mesh*/);
    }
    mesher(const shell_mesh_type& mesh) {
        m_shell_mesh = mesh;
    }
    mesher(shell_mesh_type&& mesh) {
        m_shell_mesh = std::move(mesh);
    }


private:
    real_type m_preferred_length;
    genparams<polytope_type> m_gen_params;
    genparams<polytope_type> m_current_gen_params;

    shell_type m_shell;
    shell_mesh_type m_shell_mesh;
    mesh_type m_mesh;

    // methods...
};


template <typename Polytope, template <typename... Args> typename Pointer>
class mesher<spt::multi<Polytope>, spt::simplex, Pointer> {
    using vertex_type = spt::polytope<0, Polytope::dim, typename Polytope::real_type>;
    using edge_type = spt::polytope<1, Polytope::dim, typename Polytope::real_type>;
    using facet_type = spt::polytope<N - 1, Polytope::dim, typename Polytope::real_type>;

public:
    using polytope_type = Polytope;
    using shell_type = spt::mesh_base<Pointer, facet_type, spt::multi>;
    using shell_mesh_unit_type = spt::simplex<Polytope::n - 1, Polytope::dim, typename Polytope::real_type>;
    using shell_mesh_type = spt::raw_mesh<shell_mesh_unit_type, spt::multi>;
    using mesh_unit_type = spt::simplex<Polytope::n, Polytope::dim, typename Polytope::real_type>;
    using mesh_type = spt::raw_mesh<mesh_unit_type, spt::multi>;
    using real_type = typename polytope_type::real_type;

    void run(real_type preferred_length,
             const genparams<polytope_type>& gen_params = genparams<polytope_type>());

    mesher(const shell_type& shell, const shell_mesh_type& mesh) {
        m_shell = shell;
        m_shell_mesh = mesh;
    }
    mesher(shell_type&& shell, shell_mesh_type&& mesh) noexcept {
        m_shell = std::move(shell);
        m_shell_mesh = std::move(mesh);
    }
    mesher(const shell_type& shell) {
        m_shell = shell;
        mesher<spt::multi<facet_type>, spt::simplex> sh_mesher(shell);
        // mesh the shell...
        m_shell_mesh = std::move(/*mesh*/);
    }
    mesher(shell_type&& shell) noexcept {
        m_shell = std::move(shell);
        mesher<spt::multi<facet_type>, spt::simplex> sh_mesher(shell);
        // mesh the shell...
        m_shell_mesh = std::move(/*mesh*/);
    }
    mesher(const shell_mesh_type& mesh) {
        m_shell_mesh = mesh;
    }


private:
    real_type m_preferred_length;
    genparams<polytope_type> m_gen_params;
    genparams<polytope_type> m_current_gen_params;

    shell_type m_shell;
    shell_mesh_type m_shell_mesh;
    mesh_type m_meshes;

    // methods...
};


template <
    typename Polytope,
    template <std::size_t N, std::size_t Dim, typename Real> typename ElemType,
    std::size_t N, std::size_t Dim, typename Real,
    template <typename... Args> typename Pointer,
    template <typename Polytope> typename HowManyPolytopes>
mesher(spt::mesh_base<Pointer, spt::polytope<N - 1, Dim, Real>, HowManyPolytopes>&& poly,
       spt::mesh_base<Pointer, ElemType<N, Dim, Real>, spt::multi>&& shell_mesh)
    ->mesher<HowManyPolytopes<Polytope>, ElemType, Pointer>;

} // namespace pmg