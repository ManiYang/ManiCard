#include <QCoreApplication>
#include <gtest/gtest.h>
#include "utilities/logging.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    qInstallMessageHandler(writeMessageToStdout);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
