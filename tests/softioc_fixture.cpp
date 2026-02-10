#include "softioc_fixture.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <string>
#include <thread>

void SoftIocFixture::SetUpTestSuite() {
    setenv("EPICS_CA_AUTO_ADDR_LIST", "NO", 1);
    setenv("EPICS_CA_ADDR_LIST", "127.0.0.1", 1);
    // setenv("EPICS_CA_MAX_ARRAY_BYTES", "10485760", 1); // if needed

    ctx_->EnsureAttached();

    db_text_ = R"DB(
            record(ao, "TEST:AO") {
                field(VAL,  "0")
                field(PINI, "YES")
            }
            record(longout, "TEST:LO") {
                field(VAL,  "0")
                field(PINI, "YES")
            }
            record(stringout, "TEST:STRO") {
                field(VAL,  "")
                field(PINI, "YES")
            }
        )DB";

    runner_.Start(db_text_);

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
}

void SoftIocFixture::TearDownTestSuite() { runner_.KillIfRunning(); }

TEST_F(SoftIocFixture, SoftIocPutThenGet) {
    int rc1 = system("caput -t TEST:AO 12.3");
    ASSERT_EQ(rc1, 0);

    FILE* fp = popen("caget -t TEST:AO", "r");
    ASSERT_NE(fp, nullptr);
    char buf[256]{};
    ASSERT_TRUE(fgets(buf, sizeof(buf), fp) != nullptr);
    pclose(fp);

    std::string got = buf;
    EXPECT_TRUE(got.rfind("12.3", 0) == 0);
}
