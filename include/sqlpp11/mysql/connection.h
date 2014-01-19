/*
 * Copyright (c) 2013, Roland Bock
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef SQLPP_MYSQL_CONNECTION_H
#define SQLPP_MYSQL_CONNECTION_H

#include <string>
#include <sstream>
#include <sqlpp11/connection.h>
#include "prepared_query.h"
#include "char_result.h"
#include "bind_result.h"
#include "connection_config.h"

namespace sqlpp
{
	namespace mysql
	{
		namespace detail
		{
			class connection_handle;
		}

		class connection;

		struct serializer_t
		{
			serializer_t(const connection& db):
				_db(db)
			{}

			template<typename T>
				std::ostream& operator<<(T t)
				{
					return _os << t;
				}

			std::string escape(std::string arg);

			std::string str() const
			{
				return _os.str();
			}

			const connection& _db;
			std::stringstream _os;
		};

		class connection: public sqlpp::connection
		{
			std::unique_ptr<detail::connection_handle> _handle;
			bool _transaction_active = false;

			// direct execution
			char_result_t select_impl(const std::string& query);
			size_t insert_impl(const std::string& query);
			size_t update_impl(const std::string& query);
			size_t remove_impl(const std::string& query);

			// prepared execution
			prepared_query_t prepare_impl(const std::string& query, size_t no_of_parameters, size_t no_of_columns);
			bind_result_t run_prepared_select_impl(prepared_query_t& prepared_query);
			size_t run_prepared_insert_impl(prepared_query_t& prepared_query);
			size_t run_prepared_update_impl(prepared_query_t& prepared_query);
			size_t run_prepared_remove_impl(prepared_query_t& prepared_query);

		public:
			using _prepared_query_t = ::sqlpp::mysql::prepared_query_t;
			using _context_t = serializer_t;

			connection(const std::shared_ptr<connection_config>& config);
			~connection();
			connection(const connection&) = delete;
			connection(connection&&) = delete;
			connection& operator=(const connection&) = delete;
			connection& operator=(connection&&) = delete;

			template<typename Select>
			char_result_t select(const Select& s)
			{
				_context_t context(*this);
				interpret(s, context);
				return select_impl(context.str());
			}

			template<typename Select>
			_prepared_query_t prepare_select(Select& s)
			{
				_context_t context(*this);
				interpret(s, context);
				return prepare_impl(context.str(), s._get_no_of_parameters(), s.get_no_of_result_columns());
			}

			template<typename PreparedSelect>
			bind_result_t run_prepared_select(const PreparedSelect& s)
			{
				s._bind_params();
				return run_prepared_select_impl(s._prepared_query);
			}

			//! insert returns the last auto_incremented id (or zero, if there is none)
			template<typename Insert>
			size_t insert(const Insert& i)
			{
				_context_t context(*this);
				interpret(i, context);
				return insert_impl(context.str());
			}

			template<typename Insert>
			_prepared_query_t prepare_insert(Insert& i)
			{
				_context_t context(*this);
				interpret(i, context);
				return prepare_impl(context.str(), i._get_no_of_parameters(), 0);
			}

			template<typename PreparedInsert>
			size_t run_prepared_insert(const PreparedInsert& i)
			{
				i._bind_params();
				return run_prepared_insert_impl(i._prepared_query);
			}

			//! update returns the number of affected rows
			template<typename Update>
			size_t update(const Update& u)
			{
				_context_t context(*this);
				interpret(u, context);
				return update_impl(context.str());
			}

			template<typename Update>
			_prepared_query_t prepare_update(Update& u)
			{
				_context_t context(*this);
				interpret(u, context);
				return prepare_impl(context.str(), u._get_no_of_parameters(), 0);
			}

			template<typename PreparedUpdate>
			size_t run_prepared_update(const PreparedUpdate& u)
			{
				u._bind_params();
				return run_prepared_update_impl(u._prepared_query);
			}

			//! remove returns the number of removed rows
			template<typename Remove>
			size_t remove(const Remove& r)
			{
				_context_t context(*this);
				interpret(r, context);
				return remove_impl(context.str());
			}

			template<typename Remove>
			_prepared_query_t prepare_remove(Remove& r)
			{

				_context_t context(*this);
				interpret(r, context);
				return prepare_impl(context.str(), r._get_no_of_parameters(), 0);
			}

			template<typename PreparedRemove>
			size_t run_prepared_remove(const PreparedRemove& r)
			{
				r._bind_params();
				return run_prepared_remove_impl(r._prepared_query);
			}

			//! execute arbitrary command (e.g. create a table)
			void execute(const std::string& command);

			//! escape given string (does not quote, though)
			std::string escape(const std::string& s) const;

			//! call run on the argument
			template<typename T>
				auto run(const T& t) -> decltype(t.run(*this))
				{
					return t.run(*this);
				}

			//! call prepare on the argument
			template<typename T>
				auto prepare(const T& t) -> decltype(t.prepare(*this))
				{
					return t.prepare(*this);
				}

			//! start transaction
			void start_transaction();

			//! commit transaction (or throw transaction if the transaction has been finished already)
			void commit_transaction();

			//! rollback transaction with or without reporting the rollback (or throw if the transaction has been finished already)
			void rollback_transaction(bool report);

			//! report a rollback failure (will be called by transactions in case of a rollback failure in the destructor)
			void report_rollback_failure(const std::string message) noexcept;
		};

		inline std::string serializer_t::escape(std::string arg)
		{
			return _db.escape(arg);
		}


	}
}

#include "interpreter.h"

#endif
