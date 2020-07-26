######################################################################
# Automatically generated by qmake (2.01a) sam. janv. 13 17:20:35 2007
######################################################################

TEMPLATE = subdirs

SUBDIRS = \ # sub-project names
    paint_lib \
    core_lib \
    app \
    tests \

# build the project sequentially as listed in SUBDIRS !
CONFIG += ordered

# where to find the sub projects - give the folders
core_lib.subdir = core_lib
app.subdir      = app
tests.subdir    = tests
paint_lib.subdir = 3rdlib/paint_lib

# what subproject depends on others
app.depends      = core_lib
app.depends      = paint_lib
tests.depends    = core_lib
tests.depends    = paint_lib
core_lib.depends = paint_lib

TRANSLATIONS += translations/pencil.ts \
                translations/pencil_ar.ts \
                translations/pencil_ca.ts \
                translations/pencil_cs.ts \
                translations/pencil_da.ts \
                translations/pencil_de.ts \
                translations/pencil_el.ts \
                translations/pencil_es.ts \
                translations/pencil_et.ts \
                translations/pencil_fr.ts \
                translations/pencil_he.ts \
                translations/pencil_hu_HU.ts \
                translations/pencil_id.ts \
                translations/pencil_it.ts \
                translations/pencil_ja.ts \
                translations/pencil_kab.ts \
                translations/pencil_pl.ts \
                translations/pencil_pt.ts \
                translations/pencil_pt_BR.ts \
                translations/pencil_ru.ts \
                translations/pencil_sl.ts \
                translations/pencil_sv.ts \
                translations/pencil_tr.ts \
                translations/pencil_vi.ts \
                translations/pencil_zh_CN.ts \
                translations/pencil_zh_TW.ts

macx: LIBS += -framework AppKit
