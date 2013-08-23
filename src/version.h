#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 2
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define VERSION_STRING "recommendify (standalone) %i.%i.%i\n" \
 "\n" \
 "Copyright © 2012-2013\n" \
 "  Paul Asmuth <paul@paulasmuth.com>\n" 

#define USAGE_STRING "usage: %s " \
 "{--version|--jaccard|--cosine} " \
 "[redis_key] [item_id]\n"

#endif
