#include "minitest.h"
#include "Core/SettingsManager.h"
#include <cstdio>

static void CleanupFile() {
    _wremove(L"test_config.json");
}

TEST(SettingsManager, DefaultValues) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    EXPECT_TRUE(s.GetBool("overlay_enabled", true));
    EXPECT_FALSE(s.GetBool("nonexistent", false));
    EXPECT_EQ(s.GetInt("cursor_size", 12), 12);
    EXPECT_FLOAT_EQ(s.GetFloat("cursor_opacity", 0.8f), 0.8f);
    EXPECT_EQ(s.GetString("log_level", L"info"), L"info");
    CleanupFile();
}

TEST(SettingsManager, SetAndGetBool) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    s.SetBool("test_bool", true);
    EXPECT_TRUE(s.GetBool("test_bool", false));
    s.SetBool("test_bool", false);
    EXPECT_FALSE(s.GetBool("test_bool", true));
    CleanupFile();
}

TEST(SettingsManager, SetAndGetInt) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    s.SetInt("test_int", 42);
    EXPECT_EQ(s.GetInt("test_int", 0), 42);
    CleanupFile();
}

TEST(SettingsManager, SetAndGetFloat) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    s.SetFloat("test_float", 3.14f);
    EXPECT_FLOAT_EQ(s.GetFloat("test_float", 0.0f), 3.14f);
    CleanupFile();
}

TEST(SettingsManager, SetAndGetString) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    s.SetString("test_str", L"hello");
    EXPECT_EQ(s.GetString("test_str", L""), L"hello");
    CleanupFile();
}

TEST(SettingsManager, SaveAndLoadRoundtrip) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    s.SetBool("flag", true);
    s.SetInt("num", 99);
    s.SetFloat("pi", 3.1415f);
    s.SetString("name", L"world");
    EXPECT_TRUE(s.Save());

    SettingsManager loaded;
    EXPECT_TRUE(loaded.Load(L"test_config.json"));
    EXPECT_TRUE(loaded.GetBool("flag", false));
    EXPECT_EQ(loaded.GetInt("num", 0), 99);
    EXPECT_FLOAT_EQ(loaded.GetFloat("pi", 0.0f), 3.1415f);
    EXPECT_EQ(loaded.GetString("name", L""), L"world");
    CleanupFile();
}

TEST(SettingsManager, MissingKeyReturnsDefault) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    EXPECT_EQ(s.GetInt("no_such_key", -1), -1);
    EXPECT_EQ(s.GetString("no_key", L"default"), L"default");
    CleanupFile();
}

TEST(SettingsManager, PathReturnsCorrectValue) {
    CleanupFile();
    SettingsManager s;
    s.Load(L"test_config.json");
    EXPECT_EQ(s.Path(), L"test_config.json");
    CleanupFile();
}
