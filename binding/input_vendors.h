/*
** input_vendors.h
**
** This file is part of HiddenChest
**
** Copyright (C) 2018-2025 Kyonides-Arkanthes
*/

struct
{
  int id;
  const char *name;
}
static vendors[] =
{
  { 1118,  "Microsoft Corp."        },
  { 1133,  "Logitech Inc."          },
  { 1356,  "Sony Corp."             },
  { 1406,  "Nintendo Co. Ltd."      },
  { 2821,  "ASUSTek Computer, Inc." },
  { 3695,  "Logic3"                 },
  { 5426,  "Razer USA Ltd."         },
  { 5769,  "Razer USA Ltd."         },
  { 9571,  "ZD / ShenZhen ShanWan Technology Co." },
  { 11720, "8BitDo"                 },
  { 11925, "SCUF Gaming" }
};

static elementsN(vendors);

static int vendor_ids[] = { 1118, 1133, 1356, 1406, 2821, 3695, 5426, 5769, 9571, 11720, 11925 };