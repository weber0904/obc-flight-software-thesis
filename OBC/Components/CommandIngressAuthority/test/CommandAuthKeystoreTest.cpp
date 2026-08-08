#include <gtest/gtest.h>

#include "OBC/Components/CommandIngressAuthority/CommandAuthKeystore.hpp"

#include <fstream>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string writeKeystore(const char* body) {
    char pathTemplate[] = "/tmp/command-auth-keystore-XXXXXX";
    const int fd = mkstemp(pathTemplate);
    EXPECT_NE(fd, -1);
    if (fd != -1) {
        close(fd);
    }

    std::ofstream output(pathTemplate, std::ios::out | std::ios::trunc);
    EXPECT_TRUE(output.is_open());
    output << body;
    output.close();
    return std::string(pathTemplate);
}

void removeFile(const std::string& path) {
    if (!path.empty()) {
        std::remove(path.c_str());
    }
}

constexpr const char* VALID_KEYSTORE_TEMPLATE =
    "module_serial=FPOBCSAT00000001\n"
    "[sband]\n"
    "key_hex=101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F\n"
    "[uhf]\n"
    "key_hex=303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F\n";

}  // namespace

TEST(CommandAuthKeystore, PrefersReleaseRelativeKeystoreWhenPresent) {
    char cwdTemplate[] = "/tmp/command-auth-cwd-XXXXXX";
    const char* cwdRoot = mkdtemp(cwdTemplate);
    ASSERT_NE(cwdRoot, nullptr);
    char originalCwd[PATH_MAX] = {};
    ASSERT_NE(getcwd(originalCwd, sizeof(originalCwd)), nullptr);
    ASSERT_EQ(::mkdir((std::string(cwdRoot) + "/config").c_str(), 0755), 0);
    ASSERT_EQ(::mkdir((std::string(cwdRoot) + "/config/security").c_str(), 0755), 0);

    std::ofstream output(std::string(cwdRoot) + "/config/security/command-auth.ini", std::ios::out | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output << "module_serial=FPOBCSAT00000001\n";
    output.close();

    ASSERT_EQ(chdir(cwdRoot), 0);
    const std::string resolved = OBC::defaultCommandAuthKeystorePath();
    EXPECT_EQ(resolved, "config/security/command-auth.ini");
    ASSERT_EQ(chdir(originalCwd), 0);

    std::remove((std::string(cwdRoot) + "/config/security/command-auth.ini").c_str());
    ::rmdir((std::string(cwdRoot) + "/config/security").c_str());
    ::rmdir((std::string(cwdRoot) + "/config").c_str());
    ::rmdir(cwdRoot);
}

TEST(CommandAuthKeystore, RejectsUnexpectedLegacyTupleKeys) {
    std::string body =
        std::string("module_serial=FPOBCSAT00000001\n")
        + "[sband]\n"
        + "source_id=1\n"
        + "key_hex=101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F\n"
        + "[uhf]\n"
        + "key_hex=303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F\n";
    const std::string path = writeKeystore(body.c_str());

    OBC::CommandAuthKeystore keystore;
    std::string error;
    EXPECT_FALSE(OBC::loadCommandAuthKeystore(path, keystore, error));
    EXPECT_NE(error.find("unsupported command auth keystore key"), std::string::npos);

    removeFile(path);
}

TEST(CommandAuthKeystore, RejectsMissingKeyHex) {
    std::string body =
        std::string("module_serial=FPOBCSAT00000001\n")
        + "[sband]\n"
        + "key_hex=101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F\n"
        + "[uhf]\n";
    const std::string path = writeKeystore(body.c_str());

    OBC::CommandAuthKeystore keystore;
    std::string error;
    EXPECT_FALSE(OBC::loadCommandAuthKeystore(path, keystore, error));
    EXPECT_NE(error.find("invalid command auth keystore entry for uhf"), std::string::npos);

    removeFile(path);
}

TEST(CommandAuthKeystore, LoadsSecureBaselineKeystoreWithoutLegacyTupleFields) {
    const std::string path = writeKeystore(VALID_KEYSTORE_TEMPLATE);

    OBC::CommandAuthKeystore keystore;
    std::string error;
    EXPECT_TRUE(OBC::loadCommandAuthKeystore(path, keystore, error));
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(keystore.sband.valid);
    EXPECT_TRUE(keystore.uhf.valid);
    EXPECT_EQ(keystore.sband.keyLength, 32U);
    EXPECT_EQ(keystore.uhf.keyLength, 32U);

    removeFile(path);
}
