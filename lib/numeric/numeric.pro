include(../../settings.pri)

TEMPLATE = lib
CONFIG += staticlib create_prl
INCLUDEPATH += include
HEADERS = \
    include/numeric/id.hpp \
    include/numeric/ii.hpp \
    include/numeric/nn.hpp \
    include/numeric/wk.hpp \
    include/numeric/tlsf.hpp
SOURCES = src/id.cpp
