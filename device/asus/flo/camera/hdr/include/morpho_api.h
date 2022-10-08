/**
 * @file     morpho_api.h
 * @brief    API関数定義のマクロ
 * @version  1.0.0
 * @date     Tue Sep 21 17:37:35 2010
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_API_H
#define MORPHO_API_H

/** 
 * API関数を定義するときに使用.
 * WindowsでDLLを作成する際等に書き換えることで切り替え可能
 */
#if defined(MORPHO_DLL) && defined(_WIN32)
#define MORPHO_API(type) __declspec(dllexport) extern type
#else
#define MORPHO_API(type) extern type
#endif

#endif /* #ifndef MORPHO_API_H */
