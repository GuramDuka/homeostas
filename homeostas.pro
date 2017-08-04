ROOT = $$PWD

include(settings.pri)

TEMPLATE = subdirs
SUBDIRS = \
	lib/numeric \
	lib/sqlite \
	lib/sqlite3pp \
	lib/jsoncpp \
	app
CONFIG += ordered
app.depends = sqlite sqlite3pp numeric jsoncpp
