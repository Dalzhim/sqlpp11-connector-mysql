/*
 * Copyright (c) 2013 - 2016, Roland Bock
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TabSample.h"
#include <sqlpp11/alias_provider.h>
#include <sqlpp11/functions.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/multi_column.h>
#include <sqlpp11/mysql/connection.h>
#include <sqlpp11/remove.h>
#include <sqlpp11/select.h>
#include <sqlpp11/transaction.h>
#include <sqlpp11/update.h>

#include <iostream>
#include <vector>

namespace mysql = sqlpp::mysql;
int main()
{
  auto config = std::make_shared<mysql::connection_config>();
  config->user = "root";
  config->database = "sqlpp_mysql";
  config->debug = true;
  try
  {
    mysql::connection db(config);
  }
  catch (const sqlpp::exception& e)
  {
    std::cerr << "For testing, you'll need to create a database sqlpp_mysql for user root (no password)" << std::endl;
    std::cerr << e.what() << std::endl;
    return 1;
  }

  try
  {
    mysql::connection db(config);
    db.execute(R"(DROP TABLE IF EXISTS tab_sample)");
    db.execute(R"(CREATE TABLE tab_sample (
		alpha bigint(20) DEFAULT NULL,
			beta varchar(255) DEFAULT NULL,
			gamma bool DEFAULT NULL
			))");

    const auto tab = TabSample{};
    db(insert_into(tab).set(tab.gamma = true));
    auto i = insert_into(tab).columns(tab.beta, tab.gamma);
    i.values.add(tab.beta = "rhabarbertorte", tab.gamma = false);
    i.values.add(tab.beta = "cheesecake", tab.gamma = false);
    i.values.add(tab.beta = "kaesekuchen", tab.gamma = true);
    db(i);

    auto s = dynamic_select(db).dynamic_columns(tab.alpha).from(tab).unconditionally();
    s.selected_columns.add(tab.beta);

    for (const auto& row : db(s))
    {
      std::cerr << "row.alpha: " << row.alpha << ", row.beta: " << row.at("beta") << std::endl;
    };
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }
}
