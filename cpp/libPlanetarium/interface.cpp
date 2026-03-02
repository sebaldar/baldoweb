#include <iostream>

#include "WSsrv.hpp"
#include <pybind11/pybind11.h>


PYBIND11_MODULE(planetarium, m) {
    pybind11::class_<WSsrv>(m, "WSsrv")
        .def(pybind11::init<>())
        .def("registerClient", &WSsrv::registerClient)
        .def("render", &WSsrv::render)
        .def("handleClient", &WSsrv::handleClient)
        .def("do_sendLoop", &WSsrv::do_sendLoop)
        ;
}

