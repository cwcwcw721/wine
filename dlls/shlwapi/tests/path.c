/* Unit test suite for Path functions
 *
 * Copyright 2002 Matthew Mastracci
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

const char* TEST_URL_1 = "http://www.winehq.org/tests?date=10/10/1923";
const char* TEST_URL_2 = "http://localhost:8080/tests%2e.html?date=Mon%2010/10/1923";
const char* TEST_URL_3 = "http://foo:bar@localhost:21/internal.php?query=x&return=y";

typedef struct _TEST_URL_CANONICALIZE {
    char *url;
    DWORD flags;
    HRESULT expectret;
    char *expecturl;
} TEST_URL_CANONICALIZE;

const TEST_URL_CANONICALIZE TEST_CANONICALIZE[] = {
    /*FIXME {"http://www.winehq.org/tests/../tests/../..", 0, S_OK, "http://www.winehq.org/"},*/
    {"http://www.winehq.org/tests/../tests", 0, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\n", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\r", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\r", 0, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\r", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests/../tests/", 0, S_OK, "http://www.winehq.org/tests/"},
    {"http://www.winehq.org/tests/../tests/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/../", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/..?query=x&return=y", 0, S_OK, "http://www.winehq.org/?query=x&return=y"},
    {"http://www.winehq.org/tests/../?query=x&return=y", 0, S_OK, "http://www.winehq.org/?query=x&return=y"},
    {"http://www.winehq.org/tests/..#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org/tests/../#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org/tests/../#example", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../#example"},
};

typedef struct _TEST_URL_ESCAPE {
    char *url;
    DWORD flags;
    DWORD expectescaped;
    HRESULT expectret;
    char *expecturl;
} TEST_URL_ESCAPE;

const TEST_URL_ESCAPE TEST_ESCAPE[] = {
    {"http://www.winehq.org/tests0", 0, 0, S_OK, "http://www.winehq.org/tests0"},
    {"http://www.winehq.org/tests1\n", 0, 0, S_OK, "http://www.winehq.org/tests1%0A"},
    {"http://www.winehq.org/tests2\r", 0, 0, S_OK, "http://www.winehq.org/tests2%0D"},
    {"http://www.winehq.org/tests3\r", URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, 0, S_OK, "http://www.winehq.org/tests3\r"},
    {"http://www.winehq.org/tests4\r", URL_ESCAPE_SPACES_ONLY, 0, S_OK, "http://www.winehq.org/tests4\r"},
    {"http://www.winehq.org/tests5\r", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY, 0, S_OK, "http://www.winehq.org/tests5\r"},
    {"/direct/swhelp/series6/6.2i_latestservicepack.dat\r", URL_ESCAPE_SPACES_ONLY, 0, S_OK, "/direct/swhelp/series6/6.2i_latestservicepack.dat\r"},

    {"file://////foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file://///foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:////foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:///localhost/foo/bar\\baz", 0, 0, S_OK, "file:///localhost/foo/bar/baz"},
    {"file:///foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file://loCalHost/foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file://foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:/localhost/foo/bar\\baz", 0, 0, S_OK, "file:///localhost/foo/bar/baz"},
    {"file:/foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:foo/bar\\baz", 0, 0, S_OK, "file:foo/bar/baz"},
    {"file:\\foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:\\\\foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:\\\\\\foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:\\\\localhost\\foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:///f oo/b?a r\\baz", 0, 0, S_OK, "file:///f%20oo/b?a r\\baz"},
    {"file:///foo/b#a r\\baz", 0, 0, S_OK, "file:///foo/b%23a%20r/baz"},
    {"file:///f o^&`{}|][\"<>\\%o/b#a r\\baz", 0, 0, S_OK, "file:///f%20o%5E%26%60%7B%7D%7C%5D%5B%22%3C%3E/%o/b%23a%20r/baz"},
    {"file:///f o%o/b?a r\\b%az", URL_ESCAPE_PERCENT, 0, S_OK, "file:///f%20o%25o/b?a r\\b%az"},
    {"file:/foo/bar\\baz", URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "file:%2Ffoo%2Fbar%5Cbaz"},

    {"foo/b%ar\\ba?z\\", URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "foo%2Fb%ar%5Cba%3Fz%5C"},
    {"foo/b%ar\\ba?z\\", URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "foo%2Fb%25ar%5Cba%3Fz%5C"},
    {"foo/bar\\ba?z\\", 0, 0, S_OK, "foo/bar%5Cba?z\\"},
    {"/foo/bar\\ba?z\\", 0, 0, S_OK, "/foo/bar%5Cba?z\\"},
    {"/foo/bar\\ba#z\\", 0, 0, S_OK, "/foo/bar%5Cba#z\\"},
    {"/foo/%5C", 0, 0, S_OK, "/foo/%5C"},
    {"/foo/%5C", URL_ESCAPE_PERCENT, 0, S_OK, "/foo/%255C"},

    {"http://////foo/bar\\baz", 0, 0, S_OK, "http://////foo/bar/baz"},
    {"http://///foo/bar\\baz", 0, 0, S_OK, "http://///foo/bar/baz"},
    {"http:////foo/bar\\baz", 0, 0, S_OK, "http:////foo/bar/baz"},
    {"http:///foo/bar\\baz", 0, 0, S_OK, "http:///foo/bar/baz"},
    {"http://localhost/foo/bar\\baz", 0, 0, S_OK, "http://localhost/foo/bar/baz"},
    {"http://foo/bar\\baz", 0, 0, S_OK, "http://foo/bar/baz"},
    {"http:/foo/bar\\baz", 0, 0, S_OK, "http:/foo/bar/baz"},
    {"http:foo/bar\\ba?z\\", 0, 0, S_OK, "http:foo%2Fbar%2Fba?z\\"},
    {"http:foo/bar\\ba#z\\", 0, 0, S_OK, "http:foo%2Fbar%2Fba#z\\"},
    {"http:\\foo/bar\\baz", 0, 0, S_OK, "http:/foo/bar/baz"},
    {"http:\\\\foo/bar\\baz", 0, 0, S_OK, "http://foo/bar/baz"},
    {"http:\\\\\\foo/bar\\baz", 0, 0, S_OK, "http:///foo/bar/baz"},
    {"http:\\\\\\\\foo/bar\\baz", 0, 0, S_OK, "http:////foo/bar/baz"},
    {"http:/fo ?o/b ar\\baz", 0, 0, S_OK, "http:/fo%20?o/b ar\\baz"},
    {"http:fo ?o/b ar\\baz", 0, 0, S_OK, "http:fo%20?o/b ar\\baz"},
    {"http:/foo/bar\\baz", URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "http:%2Ffoo%2Fbar%5Cbaz"},

    {"https://foo/bar\\baz", 0, 0, S_OK, "https://foo/bar/baz"},
    {"https:/foo/bar\\baz", 0, 0, S_OK, "https:/foo/bar/baz"},
    {"https:\\foo/bar\\baz", 0, 0, S_OK, "https:/foo/bar/baz"},

    {"foo:////foo/bar\\baz", 0, 0, S_OK, "foo:////foo/bar%5Cbaz"},
    {"foo:///foo/bar\\baz", 0, 0, S_OK, "foo:///foo/bar%5Cbaz"},
    {"foo://localhost/foo/bar\\baz", 0, 0, S_OK, "foo://localhost/foo/bar%5Cbaz"},
    {"foo://foo/bar\\baz", 0, 0, S_OK, "foo://foo/bar%5Cbaz"},
    {"foo:/foo/bar\\baz", 0, 0, S_OK, "foo:/foo/bar%5Cbaz"},
    {"foo:foo/bar\\baz", 0, 0, S_OK, "foo:foo%2Fbar%5Cbaz"},
    {"foo:\\foo/bar\\baz", 0, 0, S_OK, "foo:%5Cfoo%2Fbar%5Cbaz"},
    {"foo:/foo/bar\\ba?\\z", 0, 0, S_OK, "foo:/foo/bar%5Cba?\\z"},
    {"foo:/foo/bar\\ba#\\z", 0, 0, S_OK, "foo:/foo/bar%5Cba#\\z"},

    {"mailto:/fo/o@b\\%a?\\r.b#\\az", 0, 0, S_OK, "mailto:%2Ffo%2Fo@b%5C%a%3F%5Cr.b%23%5Caz"},
    {"mailto:fo/o@b\\%a?\\r.b#\\az", 0, 0, S_OK, "mailto:fo%2Fo@b%5C%a%3F%5Cr.b%23%5Caz"},
    {"mailto:fo/o@b\\%a?\\r.b#\\az", URL_ESCAPE_PERCENT, 0, S_OK, "mailto:fo%2Fo@b%5C%25a%3F%5Cr.b%23%5Caz"},

    {"ftp:fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:fo%2Fo@bar.baz%2Ffoo%2Fbar"},
    {"ftp:/fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:/fo/o@bar.baz/foo/bar"},
    {"ftp://fo/o@bar.baz/fo?o\\bar", 0, 0, S_OK, "ftp://fo/o@bar.baz/fo?o\\bar"},
    {"ftp://fo/o@bar.baz/fo#o\\bar", 0, 0, S_OK, "ftp://fo/o@bar.baz/fo#o\\bar"},
    {"ftp://localhost/o@bar.baz/fo#o\\bar", 0, 0, S_OK, "ftp://localhost/o@bar.baz/fo#o\\bar"},
    {"ftp:///fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:///fo/o@bar.baz/foo/bar"},
    {"ftp:////fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:////fo/o@bar.baz/foo/bar"}
};

typedef struct _TEST_URL_COMBINE {
    char *url1;
    char *url2;
    DWORD flags;
    HRESULT expectret;
    char *expecturl;
} TEST_URL_COMBINE;

const TEST_URL_COMBINE TEST_COMBINE[] = {
    {"http://www.winehq.org/tests", "tests1", 0, S_OK, "http://www.winehq.org/tests1"},
    /*FIXME {"http://www.winehq.org/tests", "../tests2", 0, S_OK, "http://www.winehq.org/tests2"},*/
    {"http://www.winehq.org/tests/", "../tests3", 0, S_OK, "http://www.winehq.org/tests3"},
    {"http://www.winehq.org/tests/../tests", "tests4", 0, S_OK, "http://www.winehq.org/tests4"},
    {"http://www.winehq.org/tests/../tests/", "tests5", 0, S_OK, "http://www.winehq.org/tests/tests5"},
    {"http://www.winehq.org/tests/../tests/", "/tests6/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/..", "tests7/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/?query=x&return=y", "tests8", 0, S_OK, "http://www.winehq.org/tests/tests8"},
    {"http://www.winehq.org/tests/#example", "tests9", 0, S_OK, "http://www.winehq.org/tests/tests9"},
    {"http://www.winehq.org/tests/../tests/", "/tests10/..", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests10/.."},
    {"http://www.winehq.org/tests/../", "tests11", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../tests11"},
};

static LPWSTR GetWideString(const char* szString)
{
  LPWSTR wszString = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
					 (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
  
  MultiByteToWideChar(0, 0, szString, -1, wszString, INTERNET_MAX_URL_LENGTH);

  return wszString;
}

static void FreeWideString(LPWSTR wszString)
{
   HeapFree(GetProcessHeap(), 0, wszString);
}

static void hash_url(const char* szUrl)
{
  LPCSTR szTestUrl = szUrl;
  LPWSTR wszTestUrl = GetWideString(szTestUrl);
  
  DWORD cbSize = sizeof(DWORD);
  DWORD dwHash1, dwHash2;
  ok(UrlHashA(szTestUrl, (LPBYTE)&dwHash1, cbSize) == S_OK, "UrlHashA didn't return S_OK\n");
  ok(UrlHashW(wszTestUrl, (LPBYTE)&dwHash2, cbSize) == S_OK, "UrlHashW didn't return S_OK\n");

  FreeWideString(wszTestUrl);

  ok(dwHash1 == dwHash2, "Hashes didn't compare\n");
}

static void test_UrlHash(void)
{
  hash_url(TEST_URL_1);
  hash_url(TEST_URL_2);
  hash_url(TEST_URL_3);
}

static void test_url_part(const char* szUrl, DWORD dwPart, DWORD dwFlags, const char* szExpected)
{
  CHAR szPart[INTERNET_MAX_URL_LENGTH];
  WCHAR wszPart[INTERNET_MAX_URL_LENGTH];
  LPWSTR wszUrl = GetWideString(szUrl);
  LPWSTR wszConvertedPart;

  DWORD dwSize;

  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartA(szUrl, szPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartA for \"%s\" part 0x%08lx didn't return S_OK but \"%s\"\n", szUrl, dwPart, szPart);
  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartW(wszUrl, wszPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartW didn't return S_OK\n" );

  wszConvertedPart = GetWideString(szPart);

  ok(strcmpW(wszPart,wszConvertedPart)==0, "Strings didn't match between ascii and unicode UrlGetPart!\n");

  FreeWideString(wszUrl);
  FreeWideString(wszConvertedPart);

  /* Note that v6.0 and later don't return '?' with the query */
  ok(strcmp(szPart,szExpected)==0 ||
     (*szExpected=='?' && !strcmp(szPart,szExpected+1)),
	 "Expected %s, but got %s\n", szExpected, szPart);
}

static void test_UrlGetPart(void)
{
  test_url_part(TEST_URL_3, URL_PART_HOSTNAME, 0, "localhost");
  test_url_part(TEST_URL_3, URL_PART_PORT, 0, "21");
  test_url_part(TEST_URL_3, URL_PART_USERNAME, 0, "foo");
  test_url_part(TEST_URL_3, URL_PART_PASSWORD, 0, "bar");
  test_url_part(TEST_URL_3, URL_PART_SCHEME, 0, "http");
  test_url_part(TEST_URL_3, URL_PART_QUERY, 0, "?query=x&return=y");
}

static void test_url_escape(const char *szUrl, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwEscaped;
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR *urlW, *expected_urlW;
    dwEscaped=INTERNET_MAX_URL_LENGTH;

    ok(UrlEscapeA(szUrl, szReturnUrl, &dwEscaped, dwFlags) == dwExpectReturn, "UrlEscapeA didn't return 0x%08lx from \"%s\"\n", dwExpectReturn, szUrl);
    ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", szExpectUrl, szReturnUrl, szUrl);
    
    dwEscaped = INTERNET_MAX_URL_LENGTH;
    urlW = GetWideString(szUrl);
    expected_urlW = GetWideString(szExpectUrl);
    ok(UrlEscapeW(urlW, ret_urlW, &dwEscaped, dwFlags) == dwExpectReturn, "UrlEscapeW didn't return 0x%08lx from \"%s\"\n", dwExpectReturn, szUrl);
    WideCharToMultiByte(CP_ACP,0,ret_urlW,-1,szReturnUrl,INTERNET_MAX_URL_LENGTH,0,0);
    ok(strcmpW(ret_urlW, expected_urlW)==0, "Expected \"%s\", but got \"%s\" from \"%s\" flags %08lx\n", szExpectUrl, szReturnUrl, szUrl, dwFlags);
    FreeWideString(urlW);
    FreeWideString(expected_urlW);

}
static void test_url_canonicalize(const char *szUrl, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl = GetWideString(szUrl);
    LPWSTR wszExpectUrl = GetWideString(szExpectUrl);
    LPWSTR wszConvertedUrl;
    
    DWORD dwSize;
    
    dwSize = INTERNET_MAX_URL_LENGTH;
    ok(UrlCanonicalizeA(szUrl, szReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeA didn't return 0x%08lx\n", dwExpectReturn);
    ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected %s, but got %s\n", szExpectUrl, szReturnUrl);

    dwSize = INTERNET_MAX_URL_LENGTH;
    ok(UrlCanonicalizeW(wszUrl, wszReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeW didn't return 0x%08lx\n", dwExpectReturn);
    wszConvertedUrl = GetWideString(szReturnUrl);
    ok(strcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCanonicalize!\n");
    FreeWideString(wszConvertedUrl);
    
            
    FreeWideString(wszUrl);
    FreeWideString(wszExpectUrl);
}


static void test_UrlEscape(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_ESCAPE)/sizeof(TEST_ESCAPE[0]); i++) {
        test_url_escape(TEST_ESCAPE[i].url, TEST_ESCAPE[i].flags,
                              TEST_ESCAPE[i].expectret, TEST_ESCAPE[i].expecturl);
    }
}

static void test_UrlCanonicalize(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_CANONICALIZE)/sizeof(TEST_CANONICALIZE[0]); i++) {
        test_url_canonicalize(TEST_CANONICALIZE[i].url, TEST_CANONICALIZE[i].flags,
                              TEST_CANONICALIZE[i].expectret, TEST_CANONICALIZE[i].expecturl);
    }
}

static void test_url_combine(const char *szUrl1, const char *szUrl2, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl)
{
    HRESULT hr;
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl1 = GetWideString(szUrl1);
    LPWSTR wszUrl2 = GetWideString(szUrl2);
    LPWSTR wszExpectUrl = GetWideString(szExpectUrl);
    LPWSTR wszConvertedUrl;

    DWORD dwSize;
    DWORD dwExpectLen = lstrlen(szExpectUrl);

    hr = UrlCombineA(szUrl1, szUrl2, NULL, NULL, dwFlags);
    ok(hr == E_INVALIDARG, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, E_INVALIDARG);
    
    dwSize = 0;
    hr = UrlCombineA(szUrl1, szUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);
    
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, dwExpectReturn);
    ok(dwSize == dwExpectLen, "Got length %ld, expected %ld\n", dwSize, dwExpectLen);
    if(SUCCEEDED(hr)) {
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected %s, but got %s\n", szExpectUrl, szReturnUrl);
    }

    dwSize = 0;
    hr = UrlCombineW(wszUrl1, wszUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);
    
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineW returned 0x%08lx, expected 0x%08lx\n", hr, dwExpectReturn);
    ok(dwSize == dwExpectLen, "Got length %ld, expected %ld\n", dwSize, dwExpectLen);
    if(SUCCEEDED(hr)) {
        wszConvertedUrl = GetWideString(szReturnUrl);
        ok(strcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCombine!\n");
        FreeWideString(wszConvertedUrl);
    }

    FreeWideString(wszUrl1);
    FreeWideString(wszUrl2);
    FreeWideString(wszExpectUrl);
}

static void test_UrlCombine(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_COMBINE)/sizeof(TEST_COMBINE[0]); i++) {
        test_url_combine(TEST_COMBINE[i].url1, TEST_COMBINE[i].url2, TEST_COMBINE[i].flags,
                         TEST_COMBINE[i].expectret, TEST_COMBINE[i].expecturl);
    }
}

static void test_PathSearchAndQualify(void)
{
    WCHAR path1[] = {'c',':','\\','f','o','o',0};
    WCHAR expect1[] = {'c',':','\\','f','o','o',0};
    WCHAR path2[] = {'c',':','f','o','o',0};
    WCHAR c_drive[] = {'c',':',0}; 
    WCHAR foo[] = {'f','o','o',0}; 
    WCHAR path3[] = {'\\','f','o','o',0};
    WCHAR winini[] = {'w','i','n','.','i','n','i',0};
    WCHAR out[MAX_PATH];
    WCHAR cur_dir[MAX_PATH];
    WCHAR dot[] = {'.',0};

    /* c:\foo */
    ok(PathSearchAndQualifyW(path1, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    ok(!lstrcmpiW(out, expect1), "strings don't match\n");

    /* c:foo */
    ok(PathSearchAndQualifyW(path2, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(c_drive, MAX_PATH, cur_dir, NULL);
    PathAddBackslashW(cur_dir);
    strcatW(cur_dir, foo);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");    

    /* foo */
    ok(PathSearchAndQualifyW(foo, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(dot, MAX_PATH, cur_dir, NULL);
    PathAddBackslashW(cur_dir);
    strcatW(cur_dir, foo);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");    

    /* \foo */
    ok(PathSearchAndQualifyW(path3, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(dot, MAX_PATH, cur_dir, NULL);
    strcpyW(cur_dir + 2, path3);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");

    /* win.ini */
    ok(PathSearchAndQualifyW(winini, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    if(!SearchPathW(NULL, winini, NULL, MAX_PATH, cur_dir, NULL))
        GetFullPathNameW(winini, MAX_PATH, cur_dir, NULL);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");

}

START_TEST(path)
{
  test_UrlHash();
  test_UrlGetPart();
  test_UrlCanonicalize();
  test_UrlEscape();
  test_UrlCombine();

  test_PathSearchAndQualify();
}
