# ROM sections and their addresses in the stock J5V05L19 image.
C_REGISTER_HELPERS := a33a a381 a3f9
C_IGNITION := 280e 3f74 3f95 3fad 3fbb 3fef 400d 4042 406d 4081 430d
C_INTERPOLATION := a5ff a611
C_SIGNED_BOUNDS := a412 a42f
C_FUNCTIONS := 00db 00e0 0247 02c1 0390 04b4 060a 06fe 078c 07a4 084a 0916 09c0 0a99 0bbd 0c47 0daa \
    0ea8 0fa5 109b 1272 1347 13e0 152a 1638 1702 17e7 19b4 1b56 1d15 1db8 1e88 1fe6 2041 204d 2102 \
    218d 2291 2328 240e 2474 252b 26d2 277e 27d3 2818 28b5 28f7 2990 29d0 2a4a 2a91 2b1b 2b42 2bcd \
    2be6 2c3f 2c6c 2cff 2e0e 2e9e 2f70 2ff0 30c3 3187 324f 328f 32f2 3396 33c2 34a3 3561 35c9 3630 \
    36de 37aa 381d 38bd 3927 39f2 3a85 3aea 3bb1 3c2b 3c8b 3db5 3edb $(C_IGNITION) 4077 409a 4119 \
    41aa 421a 428d 433a 4383 4427 4453 44db 454e 4598 461f 47b1 47d4 4866 4912 49cc 4a95 4b1a 4b81 \
    4c5b 4cd2 4d49 4dfb 4e6a 4ef9 4fab 507f 50fa 518f 521b 53b1 543c 54e0 5616 5736 581c 58dd 592e \
    5952 596e 5a25 9bb9 9bca 9dac 9dc8 9e62 9ed3 9f32 a020 a0b3 a153 a1de a22e a23a a2db a314 \
    $(C_REGISTER_HELPERS) a352 a39e a3d9 $(C_SIGNED_BOUNDS) a429 a43e a4dc a575 a59d \
    $(C_INTERPOLATION) a62f a69b a6cd a6ef a7e3 a822 a8b7 a962 a96f aa28 abca ac05 ac2d ac3c acea \
    ad95 ae0a b08e b17c b24c b2ed b3d9 b5de b5ef b645 b73f b7b9 b81c b943 b977 bb42 bd97 be7f c010 \
    c11c c1ad c232 c274 c2c0 c31b c33d c3f1 c470 c550 c573 c59a c640 c696 c6e5 c713 c930 ca00 caad \
    cb89 cc16 cc2d ccaf cd7c cd7f cddc ce19 ce54 ce94 ceaf cf78 cf92 cfa3 cfe8 d022 d139 d27f d36c \
    d52f d650 d697 d6ff d78c d7d5 e000 e21e e2a5 e323 e35e e41b e453 e470 e665 e6b0 e70c e763 e7c0 \
    e978 e9de ea58 eae0 eb0f ebc1 ec0c ed13 ed22 eda9 edc2 edf1 edf8 eec6 ef83 efb0 f063 f090 f094 \
    f0db f154 f1a1 f225 f275 f278 f286
C_ENTRY_VECTORS := 0000 0003 001b 0023 006b
C_VECTORS := 000b 0013 002b 0043 004b 0053 005b 0063 0083 0093 009b 00a3 00ab 00d3
C_DATA := 00b0 00d4 00de 4a89 580c 597a 5a23 5f50 5f55 5f56 5f57 5f58 5f59 5f5a 5f5b 5f5c 5f5d 5f5e \
    5f5f 5f60 5f61 5f62 5f64 5f65 5f66 5f67 5f6a 5f6b 5f6c 5f6d 5f6e 5f6f 5f70 5f71 5f72 5f73 5f74 \
    5f75 5f76 5f77 5f7d 5f7e 5f7f 5f80 5f81 5f84 5f85 5f86 5f87 5f88 5f89 5f8a 5f8b 5f8c 5f8d 5f8e \
    5f8f 5f90 5f91 5f93 5f94 5f96 5f97 5f99 5f9a 5f9b 5f9c 5f9d 5fa0 5fa1 5fa2 5fa3 5fa4 5fa5 5fa6 \
    5fa7 5fa8 5faa 5fac 5fad 5fae 5faf 5fb0 5fb1 5fb2 5fb3 5fb4 5fb5 5fb7 5fba 5fbb 5fbc 5fbd 5fbe \
    5fbf 5fc0 5fc1 5fc2 5fc6 5fc8 5fd2 5fd5 5fd8 5fda 5fdb 5fdc 5fe0 5fe1 5fe2 5fe3 5fe4 5fe5 5fe6 \
    5fe7 5fe8 5fed 5ff0 5ff2 5ff5 5ff6 6000 6051 6054 6055 6057 6058 6059 605a 605c 605d 605e 605f \
    6060 6061 6062 6063 6064 6066 6067 6068 6069 606a 606b 606d 606e 6072 6074 6075 6076 6077 6078 \
    6079 607a 607b 607c 607d 607e 607f 6080 6081 6086 608a 608b 608d 608f 6091 6093 6098 609d 609e \
    60a0 60a4 60a6 60a7 60a8 60aa 60ad 60ae 60b1 60b5 60ba 60c0 60c7 60c9 60cc 60ce 60d5 60d7 60d8 \
    60da 60db 60dc 60dd 60de 60df 60e0 60e1 60e3 60e4 60e5 60e6 60e9 60ea 60eb 60ed 60ef 60f0 60f1 \
    60f2 60f3 60f4 60f5 60f8 6100 6104 6124 6137 613c 625c 62ac 63ac 63d3 64d3 65d3 6613 6aa1 6ac8 \
    6cc8 6dc8 6eef 6f0f 6f2f 6f56 6fc4 6fe4 700b 710b 726d 736d 746d 756d 75ef 76ef 77ef 780f 782f \
    7856 7877 7977 7a77 7a87 7aa7 7ac7 7bc7 7cc7 7cd7 7cf7 7df7 7e07 7f8e 7f9e 8000 8193 8293 8393 \
    83ba 852f 862f 864f 866f 8696 8796 87bd 87e4 880b 8a0b 8a32 8a59 8a99 8ac0 8b00 8c00 8c75 8e0a \
    8e2a 8f2a 8f4a 904a 914a 924a 926a 928a 92b1 93b1 93d8 93dd 945d 955d 965d 975d 977d 987d 98a4 \
    98cb 98f2 9912 9932 9952 9972 9992 99b9 ae5b af80 b080 b67d b6fd b6ff c77d d87b e6ee e7bc f294 \
    fffa fffe
