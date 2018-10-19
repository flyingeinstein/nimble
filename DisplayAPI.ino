void handlePageGetFonts() {
  String s('[');
  for(short i=0; i < (short)(sizeof(display_fonts)/sizeof(display_fonts[0])); i++) {
      if(i>0) s+=',';
      s += '"';
      s += display_fonts[i].name;
      s += '"';
  }
  s += ']';
  server.send(200, "application/json", s);
}

void handlePageGetCode() {
  String pageN = server.arg("n");
  int n = pageN.toInt();
  if(n>=0 && n < (short)(sizeof(pages)/sizeof(pages[0])))
    server.send(200, "text/plain", pages[n]);
  else
    server.send(400, "text/pain", "invalid page");
}

void handlePageSetCode() {
  String pageN = server.arg("n");
  int n = pageN.toInt();
  String code = server.arg("plain");
  if(n>=0 && n < (short)(sizeof(pages)/sizeof(pages[0]))) {
    setPageCode(n, code.c_str());
    server.send(200, "text/plain", pages[n]);
  } else
    server.send(400, "text/pain", "invalid page");
}
