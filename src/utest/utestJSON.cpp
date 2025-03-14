//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for JSON classes.
//
//----------------------------------------------------------------------------

#include "tsjson.h"
#include "tsjsonValue.h"
#include "tsjsonNumber.h"
#include "tsjsonString.h"
#include "tsjsonObject.h"
#include "tsjsonArray.h"
#include "tsjsonRunningDocument.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsIntegerUtils.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class JsonTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Simple);
    TSUNIT_DECLARE_TEST(Numbers);
    TSUNIT_DECLARE_TEST(GitHub);
    TSUNIT_DECLARE_TEST(Factory);
    TSUNIT_DECLARE_TEST(Query);
    TSUNIT_DECLARE_TEST(RunningDocumentEmpty);
    TSUNIT_DECLARE_TEST(RunningDocument);
    TSUNIT_DECLARE_TEST(Issue1353);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _tempFileName {};

    static ts::UString LoadFile(const ts::UString& filename);
    ts::UString loadTempFile() const { return LoadFile(_tempFileName); }
};

TSUNIT_REGISTER(JsonTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void JsonTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile(u".tmp.json");
    }
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void JsonTest::afterTest()
{
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}

// Load the content of a text file.
ts::UString JsonTest::LoadFile(const ts::UString& filename)
{
    ts::UStringList lines;
    TSUNIT_ASSERT(ts::UString::Load(lines, filename));
    return ts::UString::Join(lines, u"\n");
}



//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Simple)
{
    ts::json::ValuePtr jv;
    TSUNIT_ASSERT(!ts::json::Parse(jv, u"", NULLREP));
    TSUNIT_ASSERT(jv == nullptr);

    TSUNIT_ASSERT(ts::json::Parse(jv, u" null  ", CERR));
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_ASSERT(jv->isNull());

    TSUNIT_ASSERT(!ts::json::Parse(jv, u"   false  true  ", NULLREP));
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_ASSERT(jv->isFalse());

    TSUNIT_ASSERT(ts::json::Parse(jv, u"[ true, {\"ab\":67, \"foo\" : \"bar\"} ]", CERR));
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_ASSERT(jv->isArray());
    TSUNIT_EQUAL(2, jv->size());
    TSUNIT_ASSERT(jv->at(0).isTrue());
    TSUNIT_ASSERT(jv->at(1).isObject());
    TSUNIT_ASSERT(jv->at(2).isNull());
    TSUNIT_ASSERT(jv->at(2).value(u"jjj").at(3424).isNull());
    TSUNIT_EQUAL(2, jv->at(1).size());
    TSUNIT_EQUAL(67, jv->at(1).value(u"ab").toInteger());
    TSUNIT_EQUAL(u"bar", jv->at(1).value(u"foo").toString());
    TSUNIT_ASSERT(jv->at(1).value(u"ss").isNull());

    TSUNIT_EQUAL(
        u"[\n"
        u"  true,\n"
        u"  {\n"
        u"    \"ab\": 67,\n"
        u"    \"foo\": \"bar\"\n"
        u"  }\n"
        u"]",
        jv->printed());
}

TSUNIT_DEFINE_TEST(Numbers)
{
    static const ts::UChar* response = u"{\"a\": 12, \"b\": 1.23, \"c\": 1e-2, \"d\": 1E2}";

    ts::json::ValuePtr jv;
    TSUNIT_ASSERT(ts::json::Parse(jv, response, CERR));
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_ASSERT(jv->isObject());

    TSUNIT_EQUAL(ts::json::Type::Number, jv->value(u"a").type());
    TSUNIT_ASSERT(jv->value(u"a").isInteger());
    TSUNIT_EQUAL(12, jv->value(u"a").toInteger());
    TSUNIT_EQUAL(12.0, jv->value(u"a").toFloat());

    TSUNIT_EQUAL(ts::json::Type::Number, jv->value(u"a").type());
    TSUNIT_ASSERT(jv->value(u"a").isInteger());
    TSUNIT_EQUAL(12, jv->value(u"a").toInteger());
    TSUNIT_EQUAL(12.0, jv->value(u"a").toFloat());

    TSUNIT_EQUAL(ts::json::Type::Number, jv->value(u"b").type());
    TSUNIT_ASSERT(!jv->value(u"b").isInteger());
    TSUNIT_EQUAL(1, jv->value(u"b").toInteger());
    TSUNIT_EQUAL(1.23, jv->value(u"b").toFloat());

    TSUNIT_EQUAL(ts::json::Type::Number, jv->value(u"c").type());
    TSUNIT_ASSERT(!jv->value(u"c").isInteger());
    TSUNIT_EQUAL(0, jv->value(u"c").toInteger());
    TSUNIT_EQUAL(0.01, jv->value(u"c").toFloat());

    TSUNIT_EQUAL(ts::json::Type::Number, jv->value(u"d").type());
    TSUNIT_ASSERT(jv->value(u"d").isInteger());
    TSUNIT_EQUAL(100, jv->value(u"d").toInteger());
    TSUNIT_EQUAL(100.0, jv->value(u"d").toFloat());

    TSUNIT_EQUAL(
        u"{\n"
        u"  \"a\": 12,\n"
        u"  \"b\": 1.23,\n"
        u"  \"c\": 0.01,\n"
        u"  \"d\": 100\n"
        u"}",
        jv->printed());
}

TSUNIT_DEFINE_TEST(GitHub)
{
    // Typical response from GitHub:
    static const ts::UChar* response =
        u"{\n"
        u"  \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329\",\n"
        u"  \"assets_url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329/assets\",\n"
        u"  \"upload_url\": \"https://uploads.github.com/repos/tsduck/tsduck/releases/9074329/assets{?name,label}\",\n"
        u"  \"html_url\": \"https://github.com/tsduck/tsduck/releases/tag/v3.5-419\",\n"
        u"  \"id\": 9074329,\n"
        u"  \"tag_name\": \"v3.5-419\",\n"
        u"  \"target_commitish\": \"master\",\n"
        u"  \"name\": \"Version 3.5-419\",\n"
        u"  \"draft\": false,\n"
        u"  \"author\": {\n"
        u"    \"login\": \"lelegard\",\n"
        u"    \"id\": 5641922,\n"
        u"    \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"    \"gravatar_id\": \"\",\n"
        u"    \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"    \"html_url\": \"https://github.com/lelegard\",\n"
        u"    \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"    \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"    \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"    \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"    \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"    \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"    \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"    \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"    \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"    \"type\": \"User\",\n"
        u"    \"site_admin\": false\n"
        u"  },\n"
        u"  \"prerelease\": false,\n"
        u"  \"created_at\": \"2018-01-01T18:42:41Z\",\n"
        u"  \"published_at\": \"2018-01-01T22:34:10Z\",\n"
        u"  \"assets\": [\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754862\",\n"
        u"      \"id\": 5754862,\n"
        u"      \"name\": \"tsduck-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 5973796,\n"
        u"      \"download_count\": 3,\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:47Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754864\",\n"
        u"      \"id\": 5754864,\n"
        u"      \"name\": \"tsduck-devel-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 3985044,\n"
        u"      \"download_count\": 0,\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754863\",\n"
        u"      \"id\": 5754863,\n"
        u"      \"name\": \"tsduck-dev_3.5-419_amd64.deb\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 3337710,\n"
        u"      \"download_count\": 0,\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754865\",\n"
        u"      \"id\": 5754865,\n"
        u"      \"name\": \"TSDuck-Win32-3.5-419.exe\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 35325653,\n"
        u"      \"download_count\": 1,\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:56Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754866\",\n"
        u"      \"id\": 5754866,\n"
        u"      \"name\": \"TSDuck-Win64-3.5-419.exe\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 36267256,\n"
        u"      \"download_count\": 3,\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:34:06Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754861\",\n"
        u"      \"id\": 5754861,\n"
        u"      \"name\": \"tsduck_3.5-419_amd64.deb\",\n"
        u"      \"label\": null,\n"
        u"      \"uploader\": {\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"site_admin\": false\n"
        u"      },\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"size\": 3975010,\n"
        u"      \"download_count\": 1,\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:46Z\",\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb\"\n"
        u"    }\n"
        u"  ],\n"
        u"  \"tarball_url\": \"https://api.github.com/repos/tsduck/tsduck/tarball/v3.5-419\",\n"
        u"  \"zipball_url\": \"https://api.github.com/repos/tsduck/tsduck/zipball/v3.5-419\",\n"
        u"  \"body\": \"Binaries for command-line tools and plugins:\\r\\n* Windows 32 bits: [TSDuck-Win32-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe)\\r\\n* Windows 64 bits: [TSDuck-Win64-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe)\\r\\n* Fedora 64 bits: [tsduck-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb)\\r\\n* macOS: [use Homebrew](https://github.com/tsduck/homebrew-tsduck/blob/master/README.md)\\r\\n\\r\\nBinaries for development environment:\\r\\n* Windows: Included in installer (select option \\\"Development\\\")\\r\\n* Fedora 64 bits: [tsduck-devel-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck-dev_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb)\\r\\n* macOS: Included in Homebrew package\\r\\n\"\n"
        u"}\n";

    ts::json::ValuePtr jv;
    TSUNIT_ASSERT(ts::json::Parse(jv, response, CERR));
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_ASSERT(jv->isObject());
    TSUNIT_EQUAL(u"v3.5-419", jv->value(u"tag_name").toString());
    TSUNIT_EQUAL(u"https://api.github.com/repos/tsduck/tsduck/tarball/v3.5-419", jv->value(u"tarball_url").toString());
    TSUNIT_EQUAL(u"lelegard", jv->value(u"author").value(u"login").toString());
    TSUNIT_EQUAL(u"tsduck-devel-3.5-419.fc27.x86_64.rpm", jv->value(u"assets").at(1).value(u"name").toString());

    // Same as input but names are sorted in objects.
    TSUNIT_EQUAL(
        u"{\n"
        u"  \"assets\": [\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"download_count\": 3,\n"
        u"      \"id\": 5754862,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"size\": 5973796,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:47Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754862\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"content_type\": \"audio/x-pn-realaudio-plugin\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"download_count\": 0,\n"
        u"      \"id\": 5754864,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck-devel-3.5-419.fc27.x86_64.rpm\",\n"
        u"      \"size\": 3985044,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754864\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb\",\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"download_count\": 0,\n"
        u"      \"id\": 5754863,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck-dev_3.5-419_amd64.deb\",\n"
        u"      \"size\": 3337710,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:48Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754863\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe\",\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"download_count\": 1,\n"
        u"      \"id\": 5754865,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"TSDuck-Win32-3.5-419.exe\",\n"
        u"      \"size\": 35325653,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:56Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754865\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe\",\n"
        u"      \"content_type\": \"application/octet-stream\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:42Z\",\n"
        u"      \"download_count\": 3,\n"
        u"      \"id\": 5754866,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"TSDuck-Win64-3.5-419.exe\",\n"
        u"      \"size\": 36267256,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:34:06Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754866\"\n"
        u"    },\n"
        u"    {\n"
        u"      \"browser_download_url\": \"https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb\",\n"
        u"      \"content_type\": \"application/x-deb\",\n"
        u"      \"created_at\": \"2018-01-01T22:33:41Z\",\n"
        u"      \"download_count\": 1,\n"
        u"      \"id\": 5754861,\n"
        u"      \"label\": null,\n"
        u"      \"name\": \"tsduck_3.5-419_amd64.deb\",\n"
        u"      \"size\": 3975010,\n"
        u"      \"state\": \"uploaded\",\n"
        u"      \"updated_at\": \"2018-01-01T22:33:46Z\",\n"
        u"      \"uploader\": {\n"
        u"        \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"        \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"        \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"        \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"        \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"        \"gravatar_id\": \"\",\n"
        u"        \"html_url\": \"https://github.com/lelegard\",\n"
        u"        \"id\": 5641922,\n"
        u"        \"login\": \"lelegard\",\n"
        u"        \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"        \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"        \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"        \"site_admin\": false,\n"
        u"        \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"        \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"        \"type\": \"User\",\n"
        u"        \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"      },\n"
        u"      \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/assets/5754861\"\n"
        u"    }\n"
        u"  ],\n"
        u"  \"assets_url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329/assets\",\n"
        u"  \"author\": {\n"
        u"    \"avatar_url\": \"https://avatars0.githubusercontent.com/u/5641922?v=4\",\n"
        u"    \"events_url\": \"https://api.github.com/users/lelegard/events{/privacy}\",\n"
        u"    \"followers_url\": \"https://api.github.com/users/lelegard/followers\",\n"
        u"    \"following_url\": \"https://api.github.com/users/lelegard/following{/other_user}\",\n"
        u"    \"gists_url\": \"https://api.github.com/users/lelegard/gists{/gist_id}\",\n"
        u"    \"gravatar_id\": \"\",\n"
        u"    \"html_url\": \"https://github.com/lelegard\",\n"
        u"    \"id\": 5641922,\n"
        u"    \"login\": \"lelegard\",\n"
        u"    \"organizations_url\": \"https://api.github.com/users/lelegard/orgs\",\n"
        u"    \"received_events_url\": \"https://api.github.com/users/lelegard/received_events\",\n"
        u"    \"repos_url\": \"https://api.github.com/users/lelegard/repos\",\n"
        u"    \"site_admin\": false,\n"
        u"    \"starred_url\": \"https://api.github.com/users/lelegard/starred{/owner}{/repo}\",\n"
        u"    \"subscriptions_url\": \"https://api.github.com/users/lelegard/subscriptions\",\n"
        u"    \"type\": \"User\",\n"
        u"    \"url\": \"https://api.github.com/users/lelegard\"\n"
        u"  },\n"
        u"  \"body\": \"Binaries for command-line tools and plugins:\\r\\n* Windows 32 bits: [TSDuck-Win32-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win32-3.5-419.exe)\\r\\n* Windows 64 bits: [TSDuck-Win64-3.5-419.exe](https://github.com/tsduck/tsduck/releases/download/v3.5-419/TSDuck-Win64-3.5-419.exe)\\r\\n* Fedora 64 bits: [tsduck-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck_3.5-419_amd64.deb)\\r\\n* macOS: [use Homebrew](https://github.com/tsduck/homebrew-tsduck/blob/master/README.md)\\r\\n\\r\\nBinaries for development environment:\\r\\n* Windows: Included in installer (select option \\\"Development\\\")\\r\\n* Fedora 64 bits: [tsduck-devel-3.5-419.fc27.x86_64.rpm](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-devel-3.5-419.fc27.x86_64.rpm)\\r\\n* Ubuntu 64 bits: [tsduck-dev_3.5-419_amd64.deb](https://github.com/tsduck/tsduck/releases/download/v3.5-419/tsduck-dev_3.5-419_amd64.deb)\\r\\n* macOS: Included in Homebrew package\\r\\n\",\n"
        u"  \"created_at\": \"2018-01-01T18:42:41Z\",\n"
        u"  \"draft\": false,\n"
        u"  \"html_url\": \"https://github.com/tsduck/tsduck/releases/tag/v3.5-419\",\n"
        u"  \"id\": 9074329,\n"
        u"  \"name\": \"Version 3.5-419\",\n"
        u"  \"prerelease\": false,\n"
        u"  \"published_at\": \"2018-01-01T22:34:10Z\",\n"
        u"  \"tag_name\": \"v3.5-419\",\n"
        u"  \"tarball_url\": \"https://api.github.com/repos/tsduck/tsduck/tarball/v3.5-419\",\n"
        u"  \"target_commitish\": \"master\",\n"
        u"  \"upload_url\": \"https://uploads.github.com/repos/tsduck/tsduck/releases/9074329/assets{?name,label}\",\n"
        u"  \"url\": \"https://api.github.com/repos/tsduck/tsduck/releases/9074329\",\n"
        u"  \"zipball_url\": \"https://api.github.com/repos/tsduck/tsduck/zipball/v3.5-419\"\n"
        u"}",
        jv->printed());
}

TSUNIT_DEFINE_TEST(Factory)
{
    ts::json::ValuePtr jv;

    jv = ts::json::Factory(ts::json::Type::True);
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_EQUAL(ts::json::Type::True, jv->type());
    TSUNIT_ASSERT(jv->isTrue());

    jv = ts::json::Factory(ts::json::Type::Object);
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_EQUAL(ts::json::Type::Object, jv->type());
    TSUNIT_ASSERT(jv->isObject());

    jv = ts::json::Factory(ts::json::Type::String, u"abcdef");
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_EQUAL(ts::json::Type::String, jv->type());
    TSUNIT_ASSERT(jv->isString());
    TSUNIT_EQUAL(u"abcdef", jv->toString());

    jv = ts::json::Factory(ts::json::Type::Number, u"1,234");
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_EQUAL(ts::json::Type::Number, jv->type());
    TSUNIT_ASSERT(jv->isNumber());
    TSUNIT_ASSERT(jv->isInteger());
    TSUNIT_EQUAL(1234, jv->toInteger());
    TSUNIT_EQUAL(1234.0, jv->toFloat());
    TSUNIT_EQUAL(u"1234", jv->toString());

    jv = ts::json::Factory(ts::json::Type::Number, u"12.450");
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_EQUAL(ts::json::Type::Number, jv->type());
    TSUNIT_ASSERT(jv->isNumber());
    TSUNIT_ASSERT(!jv->isInteger());
    TSUNIT_EQUAL(12, jv->toInteger());
    TSUNIT_EQUAL(12.45, jv->toFloat());
    TSUNIT_EQUAL(u"12.45", jv->toString());

    jv = ts::json::Factory(ts::json::Type::Number, u"-12.450");
    TSUNIT_ASSERT(jv != nullptr);
    TSUNIT_EQUAL(ts::json::Type::Number, jv->type());
    TSUNIT_ASSERT(jv->isNumber());
    TSUNIT_ASSERT(!jv->isInteger());
    TSUNIT_EQUAL(-12, jv->toInteger());
    TSUNIT_EQUAL(-12.45, jv->toFloat());
    TSUNIT_EQUAL(u"-12.45", jv->toString());
}

TSUNIT_DEFINE_TEST(Query)
{
    ts::json::Object root;

    root.value(u"obj1", true).value(u"obj2", true).value(u"obj3", true).add(u"num4", std::make_shared<ts::json::Number>(123));
    root.value(u"obj1").value(u"obj2").value(u"obj3").add(u"str4", std::make_shared<ts::json::String>(u"abc"));
    root.value(u"obj1").add(u"arr2", std::make_shared<ts::json::Array>());
    root.value(u"obj1").value(u"arr2").set(std::make_shared<ts::json::Number>(456));
    root.value(u"obj1").value(u"arr2").set(std::make_shared<ts::json::String>(u"def"));

    TSUNIT_EQUAL(
        u"{\n"
        u"  \"obj1\": {\n"
        u"    \"arr2\": [\n"
        u"      456,\n"
        u"      \"def\"\n"
        u"    ],\n"
        u"    \"obj2\": {\n"
        u"      \"obj3\": {\n"
        u"        \"num4\": 123,\n"
        u"        \"str4\": \"abc\"\n"
        u"      }\n"
        u"    }\n"
        u"  }\n"
        u"}",
        root.printed());

    TSUNIT_EQUAL(123, root.value(u"obj1").value(u"obj2").value(u"obj3").value(u"num4").toInteger());
    TSUNIT_ASSERT(root.value(u"obj1").value(u"arr2").isArray());

    // Constant queries
    TSUNIT_ASSERT(root.query(u"foo1").isNull());
    TSUNIT_ASSERT(root.query(u"obj1").isObject());
    TSUNIT_ASSERT(root.query(u"obj1.foo").isNull());
    TSUNIT_ASSERT(root.query(u"obj1.obj2.obj3").isObject());
    TSUNIT_ASSERT(root.query(u"obj1.obj2.obj3.num4").isNumber());
    TSUNIT_ASSERT(root.query(u"obj1.obj2.obj3.str4").isString());
    TSUNIT_ASSERT(root.query(u"obj1.obj2.obj3.foo4").isNull());
    TSUNIT_ASSERT(root.query(u"obj1.arr2").isArray());
    TSUNIT_EQUAL(2, root.query(u"obj1.arr2").size());
    TSUNIT_ASSERT(root.query(u"obj1.arr2[0]").isNumber());
    TSUNIT_EQUAL(456, root.query(u"obj1.arr2[0]").toInteger());
    TSUNIT_ASSERT(root.query(u"obj1.arr2[1]").isString());
    TSUNIT_EQUAL(u"def", root.query(u"obj1.arr2[1]").toString());
    TSUNIT_ASSERT(root.query(u"obj1.arr2[2]").isNull());

    // Creation queries
    TSUNIT_ASSERT(root.query(u"foo1.foo2.foo3[2].foo4", true).isObject());
    TSUNIT_ASSERT(root.query(u"foo1").isObject());
    TSUNIT_ASSERT(root.query(u"foo1.foo2").isObject());
    TSUNIT_ASSERT(root.query(u"foo1.foo2.foo3").isArray());
    TSUNIT_EQUAL(1, root.query(u"foo1.foo2.foo3").size());
    TSUNIT_ASSERT(root.query(u"foo1.foo2.foo3[0].foo4").isObject());
    TSUNIT_ASSERT(root.query(u"foo1.foo2.foo3[1]").isNull());

    TSUNIT_ASSERT(root.query(u"foo1.foo2.foo3[].bar4", true).isObject());
    TSUNIT_EQUAL(2, root.query(u"foo1.foo2.foo3").size());
    TSUNIT_ASSERT(root.query(u"foo1.foo2.foo3[1].bar4").isObject());

    debug() << "JsonTest::testQuery:" << std::endl << root.printed() << std::endl;
}

TSUNIT_DEFINE_TEST(RunningDocumentEmpty)
{
    ts::json::RunningDocument doc(CERR);

    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(doc.open(ts::json::ValuePtr(), _tempFileName));

    doc.add(ts::json::String(u"foo"));
    doc.add(ts::json::Number(-23));
    ts::json::Object obj1;
    obj1.value(u"obj1", true).add(u"arr2", std::make_shared<ts::json::Array>());
    doc.add(obj1);
    doc.close();

    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(u"[\n"
                 u"  \"foo\",\n"
                 u"  -23,\n"
                 u"  {\n"
                 u"    \"obj1\": {\n"
                 u"      \"arr2\": [\n"
                 u"      ]\n"
                 u"    }\n"
                 u"  }\n"
                 u"]",
                 loadTempFile());
}

TSUNIT_DEFINE_TEST(RunningDocument)
{
    ts::json::RunningDocument doc(CERR);
    ts::json::ValuePtr root(new ts::json::Object());
    root->query(u"init_obj1.subobj1", true).add(u"val1", u"zeval1");
    root->query(u"init_obj1.subobj1", true).add(u"val2", u"zeval2");
    root->query(u"init_obj1.subobj2", true).add(u"val3", u"zeval3");
    root->query(u"init_obj2.subobj3[0]", true).add(u"val4", u"zeval4");
    root->query(u"init_obj2.subobj4", true).add(u"val5", u"zeval5");
    root->query(u"init_obj3.subobj5", true).add(u"val6", u"zeval6");

    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(doc.open(root, _tempFileName));

    doc.add(ts::json::String(u"foo"));
    doc.add(ts::json::Number(-23));
    ts::json::Object obj1;
    obj1.query(u"obj1.arr2[1].obj2", true).add(u"xxxx", u"yyyy");
    doc.add(obj1);
    doc.close();

    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(u"{\n"
                 u"  \"init_obj1\": {\n"
                 u"    \"subobj1\": {\n"
                 u"      \"val1\": \"zeval1\",\n"
                 u"      \"val2\": \"zeval2\"\n"
                 u"    },\n"
                 u"    \"subobj2\": {\n"
                 u"      \"val3\": \"zeval3\"\n"
                 u"    }\n"
                 u"  },\n"
                 u"  \"init_obj3\": {\n"
                 u"    \"subobj5\": {\n"
                 u"      \"val6\": \"zeval6\"\n"
                 u"    }\n"
                 u"  },\n"
                 u"  \"init_obj2\": {\n"
                 u"    \"subobj4\": {\n"
                 u"      \"val5\": \"zeval5\"\n"
                 u"    },\n"
                 u"    \"subobj3\": [\n"
                 u"      {\n"
                 u"        \"val4\": \"zeval4\"\n"
                 u"      },\n"
                 u"      \"foo\",\n"
                 u"      -23,\n"
                 u"      {\n"
                 u"        \"obj1\": {\n"
                 u"          \"arr2\": [\n"
                 u"            {\n"
                 u"              \"obj2\": {\n"
                 u"                \"xxxx\": \"yyyy\"\n"
                 u"              }\n"
                 u"            }\n"
                 u"          ]\n"
                 u"        }\n"
                 u"      }\n"
                 u"    ]\n"
                 u"  }\n"
                 u"}",
                 loadTempFile());
}

TSUNIT_DEFINE_TEST(Issue1353)
{
    ts::json::Object root;

    root.add(u"a", 0);
    root.add(u"b", 0.0);
    int i = 0;
    root.add(u"c", i);
    double f = 0.0;
    root.add(u"d", f);
    f = 0.000001;
    root.add(u"e", f);

    TSUNIT_ASSERT(root.query(u"a").isNumber());
    TSUNIT_ASSERT(root.query(u"b").isNumber());
    TSUNIT_ASSERT(root.query(u"c").isNumber());
    TSUNIT_ASSERT(root.query(u"d").isNumber());
    TSUNIT_ASSERT(root.query(u"e").isNumber());

    TSUNIT_ASSERT(root.query(u"a").isInteger());
    TSUNIT_ASSERT(root.query(u"b").isInteger());
    TSUNIT_ASSERT(root.query(u"c").isInteger());
    TSUNIT_ASSERT(root.query(u"d").isInteger());
    TSUNIT_ASSERT(!root.query(u"e").isInteger());

    f = 1.2;
    for (size_t pow = 0; pow <= 9; pow++) {
        ts::UString name(ts::UString::Format(u"f%d", pow));
        root.add(name, f / double(ts::Power10(pow)));
    }

    TSUNIT_EQUAL(u"{ \"a\": 0, \"b\": 0, \"c\": 0, \"d\": 0, \"e\": 1.0e-6, "
                 u"\"f0\": 1.2, \"f1\": 0.12, \"f2\": 0.012, \"f3\": 0.0012, \"f4\": 1.2e-4, "
                 "\"f5\": 1.2e-5, \"f6\": 1.2e-6, \"f7\": 1.2e-7, \"f8\": 1.2e-8, \"f9\": 1.2e-9 }",
                 root.oneLiner(CERR));
}
