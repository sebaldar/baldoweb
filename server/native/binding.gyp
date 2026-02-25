{
  "targets": [
    {
      "target_name": "solar",
      "sources": [ "solar.cpp" ],
      "include_dirs": [
        "../../cpp/libPlanetarium",
        "../../cpp/libXML/include",
        "../../cpp/libJSON/include",
        "../../cpp/libSystem",
        "../../cpp/Utils",
        "../../cpp/geometria/include"
      ],
      "libraries": [
        "/usr/lib/libPlanetarium.so",
        "/usr/lib/libsystem.so",
        "/usr/lib/libXML.so",
        "/usr/lib/libJSON.so",
        "/usr/lib/libMYSQL.so",
        "-lcurl",
        "-lmariadb"
      ],
      "cflags_cc": [
        "-std=c++17",
        "-fPIC"
      ]
    }
  ]
}
