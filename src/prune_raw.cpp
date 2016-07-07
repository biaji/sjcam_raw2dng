/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#include "FileFinder.h"
#include "utils.h"

static int handle_arg(const char *arg, FileFinder &finder)
{
  struct stat sb;

  int ret = stat(arg, &sb);
  if (ret) {
    perror("stat");
    return ret;
  }

  std::string str_arg(arg);

  switch (sb.st_mode & S_IFMT) {
  case S_IFDIR:
    ret = finder.find_files(str_arg);
    break;

  default:
    fprintf(stderr, "Only directories are supported\n");
    ret = -1;
  }

  return ret;
}

static void usage(const char *prog)
{
  fprintf(stderr,
          "Usage:  %s [options] dir\n"
          "\n"
          "Valid options:\n"
          "\t-h, --help          Help\n"
          "\t-i, --input <DIR>   Directory containing .dng files if they are not\n"
          "\t                    in the same folder with .RAW and .JPG\n",
          prog);
}

static int do_prune(const std::vector<RawWorkItem *> &o_WorkItems, std::list<std::string> &files)
{
  std::vector<RawWorkItem *>::const_iterator it;

  for (it = o_WorkItems.begin(); it != o_WorkItems.end(); ++it)
    printf("Found: %s, %s\n", (*it)->m_szRawFile.c_str(), (*it)->m_szMetadataFile.c_str());

  std::list<std::string>::const_iterator dng_it;
  for (dng_it = files.begin(); dng_it != files.end(); ++dng_it)
    printf("Found DNG: %s\n", dng_it->c_str());

  return 0;
}

int main(int argc, char *argv[])
{
  if (argc == 1) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  std::string szInputFolder;
  int index;

  for (index = 1; index < argc && argv[index][0] == '-'; index++) {
    std::string option(&argv[index][1]);

    if (option == "h" || option == "-help") {
      usage(argv[0]);
      return EXIT_SUCCESS;
    } else if (option == "i" || option == "-input") {
      if (index + 1 < argc) {
        szInputFolder = argv[++index];
        struct stat sb;
        int ret = stat(szInputFolder.c_str(), &sb);
        if (ret) {
          perror("stat");
          return EXIT_FAILURE;
        }

        if (!S_ISDIR(sb.st_mode)) {
          fprintf(stderr, "Input directory not a directory\n");
          return EXIT_FAILURE;
        }

      } else {
        fprintf(stderr, "Error: Missing directory name\n");
        return EXIT_FAILURE;
      }
    } else {
      fprintf(stderr, "Error: Unknown option \"-%s\"\n", option.c_str());
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (index == argc) {
    fprintf(stderr, "Error: No directory specified\n");
    return EXIT_FAILURE;
  }

  if (index + 1 != argc) {
    fprintf(stderr, "Error: More than one directory specified\n");
    return EXIT_FAILURE;
  }

  FileFinder oFiles;

  int rc = handle_arg(argv[index], oFiles);
  if (rc)
    return EXIT_FAILURE;

  if (szInputFolder.empty())
    szInputFolder = argv[index];

  const std::vector<RawWorkItem *> o_WorkItems = oFiles.get_work_items();
  if (o_WorkItems.size() == 0) {
    printf("No raw files found in folder '%s'\n", argv[index]);
    return EXIT_SUCCESS;
  }

  std::list<std::string> filter;
  filter.push_back(dng_suffix);

  std::list<std::string> files;
  rc = list_dir(szInputFolder, files, filter);
  if (rc)
    return EXIT_FAILURE;

  if (files.size() == 0) {
    printf("No DNG files found in folder '%s'\n", szInputFolder.c_str());
    return EXIT_SUCCESS;
  }

  rc = do_prune(o_WorkItems, files);
  if (rc)
    return EXIT_FAILURE;

  printf("Prune complete\n");

  return EXIT_SUCCESS;
}