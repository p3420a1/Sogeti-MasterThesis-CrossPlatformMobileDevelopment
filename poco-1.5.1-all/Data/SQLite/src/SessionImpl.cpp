//
// SessionImpl.cpp
//
// $Id: //poco/Main/Data/SQLite/src/SessionImpl.cpp#5 $
//
// Library: SQLite
// Package: SQLite
// Module:  SessionImpl
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/Data/SQLite/SessionImpl.h"
#include "Poco/Data/SQLite/Utility.h"
#include "Poco/Data/SQLite/SQLiteStatementImpl.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/SQLiteException.h"
#include "Poco/Data/Session.h"
#include "Poco/ActiveMethod.h"
#include "Poco/ActiveResult.h"
#include "Poco/String.h"
#include "Poco/Data/DataException.h"
#include "sqlite3.h"
#include <cstdlib>


namespace Poco {
namespace Data {
namespace SQLite {


const std::string SessionImpl::DEFERRED_BEGIN_TRANSACTION("BEGIN DEFERRED");
const std::string SessionImpl::COMMIT_TRANSACTION("COMMIT");
const std::string SessionImpl::ABORT_TRANSACTION("ROLLBACK");


SessionImpl::SessionImpl(const std::string& fileName, std::size_t loginTimeout):
	Poco::Data::AbstractSessionImpl<SessionImpl>(fileName, loginTimeout),
	_connector(Connector::KEY),
	_pDB(0),
	_connected(false),
	_isTransaction(false)
{
	open();
	setConnectionTimeout(CONNECTION_TIMEOUT_DEFAULT);
	setProperty("handle", _pDB);
}


SessionImpl::~SessionImpl()
{
	close();
}


Poco::Data::StatementImpl* SessionImpl::createStatementImpl()
{
	poco_check_ptr (_pDB);
	return new SQLiteStatementImpl(*this, _pDB);
}


void SessionImpl::begin()
{
	SQLiteStatementImpl tmp(*this, _pDB);
	tmp.add(DEFERRED_BEGIN_TRANSACTION);
	tmp.execute();
	_isTransaction = true;
}


void SessionImpl::commit()
{
	SQLiteStatementImpl tmp(*this, _pDB);
	tmp.add(COMMIT_TRANSACTION);
	tmp.execute();
	_isTransaction = false;
}


void SessionImpl::rollback()
{
	SQLiteStatementImpl tmp(*this, _pDB);
	tmp.add(ABORT_TRANSACTION);
	tmp.execute();
	_isTransaction = false;
}


void SessionImpl::setTransactionIsolation(Poco::UInt32 ti)
{
	if (ti != Session::TRANSACTION_READ_COMMITTED)
		throw Poco::InvalidArgumentException("setTransactionIsolation()");
}


Poco::UInt32 SessionImpl::getTransactionIsolation()
{
	return Session::TRANSACTION_READ_COMMITTED;
}


bool SessionImpl::hasTransactionIsolation(Poco::UInt32 ti)
{
	if (ti == Session::TRANSACTION_READ_COMMITTED) return true;
	return false;
}


bool SessionImpl::isTransactionIsolation(Poco::UInt32 ti)
{
	if (ti == Session::TRANSACTION_READ_COMMITTED) return true;
	return false;
}


class ActiveConnector
{
public:
	ActiveConnector(const std::string& connectString, sqlite3** ppDB):
		connect(this, &ActiveConnector::connectImpl),
		_connectString(connectString),
		_ppDB(ppDB)
	{
		poco_check_ptr(_ppDB);
	}
	
	ActiveMethod<int, void, ActiveConnector> connect;
	
private:
	ActiveConnector();

	inline int connectImpl()
	{
		return sqlite3_open(_connectString.c_str(), _ppDB);
	}

	std::string _connectString;
	sqlite3**   _ppDB;
};


void SessionImpl::open(const std::string& connect)
{
	if (connect != connectionString())
	{
		if (isConnected())
			throw InvalidAccessException("Session already connected");

		if (!connect.empty())
			setConnectionString(connect);
	}

	poco_assert_dbg (!connectionString().empty());

	try
	{
		ActiveConnector connector(connectionString(), &_pDB);
		ActiveResult<int> result = connector.connect();
		if (!result.tryWait(getLoginTimeout() * 1000))
			throw ConnectionFailedException("Timed out.");

		int rc = result.data();
		if (rc != 0)
		{
			close();
			Utility::throwException(rc);
		}
	} catch (SQLiteException& ex)
	{
		throw ConnectionFailedException(ex.displayText());
	}

	if (SQLITE_OK != sqlite3_exec(_pDB,
		"attach database ':memory:' as sys;"
		"create table sys.dual (dummy);", 
		0, 0, 0))
	{
		throw ExecutionException("Cannot create system database.");
	}

	_connected = true;
}


void SessionImpl::close()
{
	if (_pDB)
	{
		sqlite3_close(_pDB);
		_pDB = 0;
	}

	_connected = false;
}


bool SessionImpl::isConnected()
{
	return _connected;
}


void SessionImpl::setConnectionTimeout(std::size_t timeout)
{
	int tout = 1000 * timeout;
	int rc = sqlite3_busy_timeout(_pDB, tout);
	if (rc != 0) Utility::throwException(rc);
	_timeout = tout;
}


} } } // namespace Poco::Data::SQLite