#include <catch2/catch_all.hpp>
#include <cstring>

#include "../expr/BVConst.hpp"

using namespace naaz::expr;

TEST_CASE("IntConst constuctor 1", "[intconst]")
{
    BVConst c((const uint8_t*)"\xaa\xbb\xcc\xdd", 4L);

    REQUIRE(c.as_u64() == 0xaabbccdd);
}

TEST_CASE("IntConst constuctor 2", "[intconst]")
{
    BVConst c((const uint8_t*)"\xaa\xbb\xcc\xdd", 4L, naaz::Endianess::LITTLE);

    REQUIRE(c.as_u64() == 0xddccbbaa);
}

TEST_CASE("IntConst constuctor 3", "[intconst]")
{
    BVConst c(
        (const uint8_t*)"\x8a\xed\xaa\xaf\x73\xab\x27\x84\x6a\x0d\x82\xb1\x91"
                        "\x4d\x9b\x89\x9e\x70\x22\x6f\xcc\xf2\x2c\x0a\x91\xa5"
                        "\xe7\xce\x39\xb6\x46\x2d\x28\xf3\x84\x66\x00\x98\x1d"
                        "\xe8\x80\x75\x29\x51\x91\x9a\xf8\x16\x9e\x12\x0f\x25"
                        "\x6e\x79\x5e\x50\x7d\x8f\x28\xac\x2a\xc5\x9a\xa9\xf3"
                        "\x9f\x28\xbd\x8a\x21\xdb\x62\xca\xa4\xe4\x32\x6e\x4f"
                        "\xba\x3f\x70\xba\x35\x5f\xef\x9b\x9e\x3c\xc2\xf0\x27",
        91L);

    REQUIRE(c.to_string(true) ==
            "0x8aedaaaf73ab27846a0d82b1914d9b899e70226fccf22c0a91a5e7ce39b6462d"
            "28f3846600981de880752951919af8169e120f256e795e507d8f28ac2ac59aa9f3"
            "9f28bd8a21db62caa4e4326e4fba3f70ba355fef9b9e3cc2f027");
}

TEST_CASE("IntConst constuctor 4", "[intconst]")
{
    BVConst c(
        (const uint8_t*)"\x8a\xed\xaa\xaf\x73\xab\x27\x84\x6a\x0d\x82\xb1\x91"
                        "\x4d\x9b\x89\x9e\x70\x22\x6f\xcc\xf2\x2c\x0a\x91\xa5"
                        "\xe7\xce\x39\xb6\x46\x2d\x28\xf3\x84\x66\x00\x98\x1d"
                        "\xe8\x80\x75\x29\x51\x91\x9a\xf8\x16\x9e\x12\x0f\x25"
                        "\x6e\x79\x5e\x50\x7d\x8f\x28\xac\x2a\xc5\x9a\xa9\xf3"
                        "\x9f\x28\xbd\x8a\x21\xdb\x62\xca\xa4\xe4\x32\x6e\x4f"
                        "\xba\x3f\x70\xba\x35\x5f\xef\x9b\x9e\x3c\xc2\xf0\x27",
        91L, naaz::Endianess::LITTLE);

    REQUIRE(c.to_string(true) ==
            "0x27f0c23c9e9bef5f35ba703fba4f6e32e4a4ca62db218abd289ff3a99ac52aac"
            "288f7d505e796e250f129e16f89a9151297580e81d98006684f3282d46b639cee7"
            "a5910a2cf2cc6f22709e899b4d91b1820d6a8427ab73afaaed8a");
}

TEST_CASE("IntConst add 1", "[intconst]")
{
    BVConst c1(10, 256);
    BVConst c2(1231, 256);

    c1.add(c2);
    REQUIRE(c1.to_string().compare("1241") == 0);
}

TEST_CASE("IntConst sub 1", "[intconst]")
{
    BVConst c1(10, 256);
    BVConst c2(1231, 256);

    c2.sub(c1);
    REQUIRE(c2.to_string().compare("1221") == 0);
}

TEST_CASE("IntConst shl 1", "[intconst]")
{
    BVConst c(10, 32);

    c.shl(1);
    REQUIRE(c.as_u64() == 20);
}

TEST_CASE("IntConst shl 2", "[intconst]")
{
    BVConst c(10, 256);

    c.shl(1);
    REQUIRE(c.to_string().compare("20") == 0);
}

TEST_CASE("IntConst lshr 1", "[intconst]")
{
    BVConst c(128, 8);

    c.lshr(1);
    REQUIRE(c.as_u64() == 64);
}

TEST_CASE("IntConst lshr 2", "[intconst]")
{
    BVConst c("0xff000000000000000000000000000000", 128);

    c.lshr(1);
    REQUIRE(c.to_string(true).compare("0x7f800000000000000000000000000000") ==
            0);
}

TEST_CASE("IntConst ashr 1", "[intconst]")
{
    BVConst c(128, 8);

    c.ashr(1);
    REQUIRE(c.as_u64() == 192);
}

TEST_CASE("IntConst ashr 2", "[intconst]")
{
    BVConst c("0xff000000000000000000000000000000", 128);

    c.ashr(1);
    REQUIRE(c.to_string(true).compare("0xff800000000000000000000000000000") ==
            0);
}

TEST_CASE("IntConst concat 1", "[intconst]")
{
    BVConst c1(1231, 256);
    BVConst c2(10, 256);
    c1.concat(c2);

    REQUIRE(c1.to_string(true).compare("0x4cf0000000000000000000000000000000000"
                                       "00000000000000000000000000000a") == 0);
}

TEST_CASE("IntConst extract 1", "[intconst]")
{
    BVConst c(0xaabbccddeeff, 256);
    c.extract(7, 0);

    REQUIRE(c.size() == 8);
    REQUIRE(c.to_string(true).compare("0xff") == 0);
}

TEST_CASE("IntConst extract 2", "[intconst]")
{
    BVConst c(
        "0xbb7b88e778162f23577e0e22f9a0438cb46e0e3f82b22d7b99bddaf09a40e818df94"
        "f46f1ecfb177bf34c7f5b5613820099bd52338fad43b5c49066adf3a16b62dd271b9b3"
        "ab99ee048663e0af30a13d268870e5abef25d9cf13fb968561b9ea85424457440d7349"
        "425721f813251d179170b8a4bb9b42e8bc50c92101c953e596b0ae1de88f7589ede394"
        "7e01b9dcd50e5e8f742dee07aaf2cac9841a83b4647ca701f9236e3bd65d6aba7a4f8b"
        "87f73d83ad6dc16f29c425e8dccb9b0837db2a44e86f2c81d9b81e269f77a9d2932527"
        "6b77176539cb9bf25f25e7f5c7afaa4d4e076225dfed119755a4b6e0254d868800fecc"
        "b0095b28a9f8d22694980aacb5ac596164466a5f8ca19068ee30e57bbb09b07e3ed96e"
        "da86a068f5008807873037397a715acbbcde233293368e775c3201a140f0d965acf5aa"
        "b9d45a690a3178ad739e1e5eab636521e04b4cb6b851c72b91b68fc19c4767877e594b"
        "a93fbebaf2c823decb047d6577f73a40560b303e67d7b7ad077ab59d7f4a155b566d69"
        "4cdc7867694782ddbd2f0d07174dc549f1f821fda2f440c8705e93ed3ad88a838b2391"
        "28e3779239a3d03274643a4c0076b9d7922bc6fc2d7e6c2635c4a4de8b66c0ed126628"
        "423d300b7c12465c96af719947dbb283d1122a0eb6b97f5a226f125a334b7b4b0bfea1"
        "9b0ebf077bb5e3f29c0471dc5c32287a52d051a6409f3064057b17a06c0fb0369e1624"
        "380a9118183d3017b593f14def838f54ec5278fef97e2ddc77f1caf52e464194dd9623"
        "eafa414fa923a27781d646c42f21edaa508e08c9f1fa32c26447e21bd9152cb24b56b4"
        "880d9bd34ca36bc232f64d869c7f287d21b19da079a3b00a6ed3710e6b5243bc220fe9"
        "56b65df7bc563b0781b805c088c796a721295cc347f53013ad26f47c4af5b039f75a23"
        "fa67038714a635306ee42706e1522ca54c0b582bfcc30b5c791f488c44b523396d71ab"
        "d43f271bec93fe47a310e52230b6025368e99c1b65c94dd01e3b9b3a7c6aec5294b9cb"
        "915ed90bc67069046e350f95a2001cd90e8699b1731a254c008f3c0c7ffbc8a13cd14e"
        "52e220369d82ad37b4dd1ac7a0028c70723d1ca254fa828079ade843cb1f12a243a6ed"
        "da4c7f432df4bdca97f6aa31e99cd21755c84af4ec61ab38e074138c1d3a7285fb3a5f"
        "209da13df9e136ac5006722ef75ac2387e933565b1d131eb5849794c17aa12277ae393"
        "a2feec51ade55b5483d7d3f32334992f609b5f789c4abd306ccfb48b24c3bd225f0939"
        "7fac7c9500c801c0388283294971cc23c7107b36fcc28008eaf10e2557c95da91f3df6"
        "676f8e8b298757df90032952c40cc732c728c06e3d1cb41c8d4f9c876d278531140ebe"
        "f8b10a1164b45236b0d34411895032086711fa48f901e4c60fed8d7ab677200b16ae31"
        "c6a1155d80f875848cb4",
        1024);
    c.extract(255, 0);

    REQUIRE(c.size() == 256);
    REQUIRE(c.to_string(true).compare("0x5032086711fa48f901e4c60fed8d7ab677200b"
                                      "16ae31c6a1155d80f875848cb4") == 0);
}

TEST_CASE("IntConst zext 1", "[intconst]")
{
    BVConst c(10, 8);
    c.zext(32);

    REQUIRE(c.size() == 32);
    REQUIRE(c.to_string().compare("10") == 0);
}

TEST_CASE("IntConst zext 2", "[intconst]")
{
    BVConst c(10, 64);
    c.zext(128);

    REQUIRE(c.size() == 128);
    REQUIRE(c.to_string().compare("10") == 0);
}

TEST_CASE("IntConst zext 3", "[intconst]")
{
    BVConst c(10, 128);
    c.zext(256);

    REQUIRE(c.size() == 256);
    REQUIRE(c.to_string().compare("10") == 0);
}

TEST_CASE("IntConst sext 1", "[intconst]")
{
    BVConst c(10, 8);
    c.sext(32);

    REQUIRE(c.size() == 32);
    REQUIRE(c.to_string().compare("10") == 0);
}

TEST_CASE("IntConst sext 2", "[intconst]")
{
    BVConst c(128, 8);
    c.sext(32);

    REQUIRE(c.size() == 32);
    REQUIRE(c.to_string().compare("4294967168") == 0);
}

TEST_CASE("IntConst sext 3", "[intconst]")
{
    BVConst c(0xffffff00, 32);
    c.sext(128);

    REQUIRE(c.size() == 128);
    REQUIRE(c.to_string(true).compare("0xffffffffffffffffffffffffffffff00") ==
            0);
}

TEST_CASE("IntConst as_data 1", "[intconst]")
{
    BVConst c(0xaabbccdd, 32);

    std::vector<uint8_t> data = c.as_data();

    REQUIRE(memcmp(data.data(), "\xaa\xbb\xcc\xdd", data.size()) == 0);
}

TEST_CASE("IntConst comparisons 1", "[intconst]")
{
    BVConst c1(0xffUL, 8);
    BVConst c2(0x01UL, 8);

    REQUIRE(c1.ugt(c2));
    REQUIRE(c1.slt(c2));
    REQUIRE(!c1.eq(c2));
}

TEST_CASE("IntConst comparisons 2", "[intconst]")
{
    BVConst c1(0xffUL, 8);
    BVConst c2(0xfeUL, 8);

    REQUIRE(c1.ugt(c2));
    REQUIRE(c1.sgt(c2));
    REQUIRE(!c1.eq(c2));
}

TEST_CASE("IntConst comparisons 3", "[intconst]")
{
    BVConst c1(0x10UL, 8);
    BVConst c2(0x20UL, 8);

    REQUIRE(c1.ult(c2));
    REQUIRE(c1.slt(c2));
    REQUIRE(!c1.eq(c2));
}

TEST_CASE("IntConst comparisons 4", "[intconst]")
{
    BVConst c1(0xffUL, 8);
    c1.sext(128);

    BVConst c2(0x01UL, 128);

    REQUIRE(c1.ugt(c2));
    REQUIRE(c1.slt(c2));
    REQUIRE(!c1.eq(c2));
}

TEST_CASE("IntConst comparisons 5", "[intconst]")
{
    BVConst c1(0xffUL, 8);
    c1.sext(128);

    BVConst c2(0xfeUL, 8);
    c2.sext(128);

    REQUIRE(c1.ugt(c2));
    REQUIRE(c1.sgt(c2));
    REQUIRE(!c1.eq(c2));
}

TEST_CASE("IntConst comparisons 6", "[intconst]")
{
    BVConst c1(0x10UL, 128);
    BVConst c2(0x20UL, 128);

    REQUIRE(c1.ult(c2));
    REQUIRE(c1.slt(c2));
    REQUIRE(!c1.eq(c2));
}
