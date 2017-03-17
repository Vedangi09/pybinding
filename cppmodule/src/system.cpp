#include "system/System.hpp"
#include "wrappers.hpp"
using namespace cpb;

void wrap_system(py::module& m) {
    py::class_<CartesianArray>(m, "CartesianArray")
        .def_property_readonly("x", [](CartesianArray const& a) { return arrayref(a.x); })
        .def_property_readonly("y", [](CartesianArray const& a) { return arrayref(a.y); })
        .def_property_readonly("z", [](CartesianArray const& a) { return arrayref(a.z); })
        .def("__getstate__", [](CartesianArray const& a) {
            return py::make_tuple(arrayref(a.x), arrayref(a.y), arrayref(a.z));
        })
        .def("__setstate__", [](CartesianArray& a, py::tuple t) {
            using T = decltype(a.x);
            new (&a) CartesianArray(t[0].cast<T>(), t[1].cast<T>(), t[2].cast<T>());
        });

    py::class_<CompressedSublattices>(m, "CompressedSublattices")
        .def("__getstate__", [](CompressedSublattices const& c) {
            return py::dict("alias_ids"_a=c.alias_ids(), "site_counts"_a=c.site_counts(),
                            "orbital_counts"_a=c.orbital_counts());
        })
        .def("__setstate__", [](CompressedSublattices& c, py::dict d) {
            new (&c) CompressedSublattices(d["alias_ids"].cast<ArrayXi>(),
                                           d["site_counts"].cast<ArrayXi>(),
                                           d["orbital_counts"].cast<ArrayXi>());
        });

    py::class_<HoppingBlocks>(m, "HoppingBlocks")
        .def("__getstate__", [](HoppingBlocks const& hb) {
            return py::dict("num_sites"_a=hb.get_num_sites(), "data"_a=hb.get_serialized_blocks());
        })
        .def("__setstate__", [](HoppingBlocks& hb, py::dict d) {
            new (&hb) HoppingBlocks(d["num_sites"].cast<idx_t>(),
                                    d["data"].cast<HoppingBlocks::SerializedBlocks>());
        });

    using Boundary = System::Boundary;
    py::class_<Boundary>(m, "Boundary")
        .def_property_readonly("hoppings", [](Boundary const& b) {
            return b.hopping_blocks.to_csr();
        })
        .def_readonly("shift", &Boundary::shift)
        .def("__getstate__", [](Boundary const& b) { return py::make_tuple(b.hopping_blocks, b.shift); })
        .def("__setstate__", [](Boundary& b, py::tuple t) {
            new (&b) Boundary{t[0].cast<decltype(b.hopping_blocks)>(),
                              t[1].cast<decltype(b.shift)>()};
        });

    py::class_<System, std::shared_ptr<System>>(m, "System")
        .def("find_nearest", &System::find_nearest, "position"_a, "sublattice"_a="")
        .def_readonly("lattice", &System::lattice)
        .def_property_readonly("positions", [](System const& s) {
            auto const& a = s.positions;
            auto type = py::module::import("pybinding.support.structure").attr("Positions");
            return type(arrayref(a.x), arrayref(a.y), arrayref(a.z));
        })
        .def_property_readonly("sublattices", [](System const& s) {
            auto type = py::module::import("pybinding.support.alias").attr("AliasArray");
            return type(s.compressed_sublattices.decompress(), s.lattice.sub_name_map());
        })
        .def_property_readonly("hoppings", [](System const& s) {
            auto type = py::module::import("pybinding.support.alias").attr("AliasCSRMatrix");
            return type(s.hopping_blocks.to_csr(), "mapping"_a=s.lattice.hop_name_map());
        })
        .def_readonly("boundaries", &System::boundaries)
        .def("__getstate__", [](System const& s) {
            return py::dict("lattice"_a=s.lattice, "positions"_a=s.positions,
                            "compressed_sublattices"_a=s.compressed_sublattices,
                            "hopping_blocks"_a=s.hopping_blocks,
                            "boundaries"_a=s.boundaries);
        })
        .def("__setstate__", [](System& s, py::dict d) {
            new (&s) System(d["lattice"].cast<decltype(s.lattice)>());
            s.positions = d["positions"].cast<decltype(s.positions)>();
            s.compressed_sublattices =
                d["compressed_sublattices"].cast<decltype(s.compressed_sublattices)>();
            s.hopping_blocks = d["hopping_blocks"].cast<decltype(s.hopping_blocks)>();
            s.boundaries = d["boundaries"].cast<decltype(s.boundaries)>();
        });
}
