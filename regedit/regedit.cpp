#include <QApplication>
#include <QPushButton>
#include "ntreg.h"

int main( int argc, char **argv )
{
	QApplication app(argc, argv);

	if (argc < 2)
	{
		fprintf(stderr, "Need the name of registry hive\n");
		return 1;
	}

	const char *filename = argv[1];

	struct hive *hive;
	hive = open_hive(filename, HMODE_RO);
	if (!hive)
	{
		fprintf(stderr, "Failed to open hive\n");
		return 1;
	}

	QPushButton hello("Hello world!");
	hello.resize(100, 30);

	hello.show();
	return app.exec();
}
