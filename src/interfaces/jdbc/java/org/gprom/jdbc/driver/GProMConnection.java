package org.gprom.jdbc.driver;

import java.sql.Array;
import java.sql.Blob;
import java.sql.CallableStatement;
import java.sql.Clob;
import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.NClob;
import java.sql.PreparedStatement;
import java.sql.SQLClientInfoException;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.SQLXML;
import java.sql.Savepoint;
import java.sql.Statement;
import java.sql.Struct;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.Executor;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.gprom.jdbc.driver.GProMJDBCUtil.BackendType;
import org.gprom.jdbc.jna.GProMWrapper;
import org.gprom.jdbc.utility.LoggerUtil;

public class GProMConnection implements GProMConnectionInterface{

	/** the wrapped connection */
	protected Connection con;
	// Variable
	private static Logger log = LogManager.getLogger(GProMConnection.class);
	private Properties gpromConf;
	private BackendType backend;
	private GProMWrapper w;

	
	public GProMConnection(Connection con, Properties gpromConf, BackendType backend, GProMWrapper w) {
		this.con = con;
		this.gpromConf = gpromConf;
		this.backend = backend;
		this.w = w;
	}
	
	public GProMStatement createGProMStatement() {
		try {
			return new GProMStatement(con.createStatement(),backend);
		} catch (SQLException e) {
			LoggerUtil.logException(e, log);
			log.error("Error creating a new gprom statement");
			System.exit(-1);
		}
		return null;
	}

	public void clearWarnings() throws SQLException {
		con.clearWarnings();
	}

	public void close() throws SQLException {
		con.close();
		w.shutdown();
	}

	public void commit() throws SQLException {
		con.commit();
	}

	public Statement createStatement() throws SQLException {
		return new GProMStatement(con.createStatement(), backend);
	}

	public Statement createStatement(int resultSetType, int resultSetConcurrency)
			throws SQLException {
		return new GProMStatement(con.createStatement(resultSetType,resultSetConcurrency), backend);
	}

	public Statement createStatement(int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		return new GProMStatement(con.createStatement(resultSetType,resultSetConcurrency,resultSetHoldability), backend);
	}

	

	public boolean getAutoCommit() throws SQLException {
		return con.getAutoCommit();
	}

	public String getCatalog() throws SQLException {
		return con.getCatalog();
	}

	

	public int getHoldability() throws SQLException {
		return con.getHoldability();
	}

	public DatabaseMetaData getMetaData() throws SQLException {
		return con.getMetaData();
	}

	public int getTransactionIsolation() throws SQLException {
		return con.getTransactionIsolation();
	}

	public Map<String, Class<?>> getTypeMap() throws SQLException {
		return con.getTypeMap();
	}

	public SQLWarning getWarnings() throws SQLException {
		return con.getWarnings();
	}

	public boolean isClosed() throws SQLException {
		return con.isClosed();
	}

	public boolean isReadOnly() throws SQLException {
		return con.isReadOnly();
	}

	public String nativeSQL(String sql) throws SQLException {
		return con.nativeSQL(sql);
	}

	public CallableStatement prepareCall(String sql) throws SQLException {
		return con.prepareCall(sql);
	}

	public CallableStatement prepareCall(String sql, int resultSetType,
			int resultSetConcurrency) throws SQLException {
		return con.prepareCall(sql, resultSetType, resultSetConcurrency);
	}

	public CallableStatement prepareCall(String sql, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		return con.prepareCall(sql, resultSetType, resultSetConcurrency, resultSetHoldability);
	}

	public PreparedStatement prepareStatement(String sql) throws SQLException {
		return con.prepareStatement(sql);
	}

	public PreparedStatement prepareStatement(String sql, int autoGeneratedKeys)
			throws SQLException {
		return con.prepareStatement(sql, autoGeneratedKeys);
	}

	public PreparedStatement prepareStatement(String sql, int[] columnIndexes)
			throws SQLException {
		return con.prepareStatement(sql, columnIndexes);
	}

	public PreparedStatement prepareStatement(String sql, String[] columnNames)
			throws SQLException {
		return con.prepareStatement(sql, columnNames);
	}

	public PreparedStatement prepareStatement(String sql, int resultSetType,
			int resultSetConcurrency) throws SQLException {
		return con.prepareStatement(sql, resultSetType, resultSetConcurrency);
	}

	public PreparedStatement prepareStatement(String sql, int resultSetType,
			int resultSetConcurrency, int resultSetHoldability)
			throws SQLException {
		return con.prepareStatement(sql, resultSetType, resultSetConcurrency, resultSetHoldability);
	}

	public void releaseSavepoint(Savepoint savepoint) throws SQLException {
		con.releaseSavepoint(savepoint);
	}

	public void rollback() throws SQLException {
		con.rollback();
	}

	public void rollback(Savepoint savepoint) throws SQLException {
		con.rollback(savepoint);
	}

	public void setAutoCommit(boolean autoCommit) throws SQLException {
		con.setAutoCommit(autoCommit);
	}

	public void setCatalog(String catalog) throws SQLException {
		con.setCatalog(catalog);
	}

	

	public void setHoldability(int holdability) throws SQLException {
		con.setHoldability(holdability);
	}

	public void setReadOnly(boolean readOnly) throws SQLException {
		con.setReadOnly(readOnly);
	}

	public Savepoint setSavepoint() throws SQLException {
		return con.setSavepoint();
	}

	public Savepoint setSavepoint(String name) throws SQLException {
		return con.setSavepoint(name);
	}

	public void setTransactionIsolation(int level) throws SQLException {
		con.setTransactionIsolation(level);
	}

	public void setTypeMap(Map<String, Class<?>> map) throws SQLException {
		con.setTypeMap(map);
	}

	public Array createArrayOf(String arg0, Object[] arg1) throws SQLException {
		return con.createArrayOf(arg0, arg1);
	}

	public Blob createBlob() throws SQLException {
		return con.createBlob();
	}

	public Clob createClob() throws SQLException {
		return con.createClob();
	}

	public NClob createNClob() throws SQLException {
		return con.createNClob();
	}

	public SQLXML createSQLXML() throws SQLException {
		return con.createSQLXML();
	}

	public Struct createStruct(String arg0, Object[] arg1) throws SQLException {
		return con.createStruct(arg0, arg1);
	}

	public Properties getClientInfo() throws SQLException {
		return con.getClientInfo();
	}

	public String getClientInfo(String arg0) throws SQLException {
		return con.getClientInfo(arg0);
	}

	public boolean isValid(int arg0) throws SQLException {
		return con.isValid(arg0);
	}

	public void setClientInfo(Properties arg0) throws SQLClientInfoException {
		con.setClientInfo(arg0);
	}

	public void setClientInfo(String arg0, String arg1)
			throws SQLClientInfoException {
		con.setClientInfo(arg0,arg1);
	}

	public boolean isWrapperFor(Class<?> iface) throws SQLException {
		return con.isWrapperFor(iface);
	}

	public <T> T unwrap(Class<T> iface) throws SQLException {
		return con.unwrap(iface);
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setSchema(java.lang.String)
	 */
	@Override
	public void setSchema(String schema) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getSchema()
	 */
	@Override
	public String getSchema() throws SQLException {
		// TODO Auto-generated method stub
		return null;
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#abort(java.util.concurrent.Executor)
	 */
	@Override
	public void abort(Executor executor) throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#setNetworkTimeout(java.util.concurrent.Executor, int)
	 */
	@Override
	public void setNetworkTimeout(Executor executor, int milliseconds)
			throws SQLException {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see java.sql.Connection#getNetworkTimeout()
	 */
	@Override
	public int getNetworkTimeout() throws SQLException {
		// TODO Auto-generated method stub
		return 0;
	}

	public Properties getGpromConf() {
		return gpromConf;
	}
	
	public void setProperty (String key, String value) {
		gpromConf.setProperty(key, value);
		
	}

	protected void setGpromConf(Properties gpromConf) {
		this.gpromConf = gpromConf;
	}

	public BackendType getBackend() {
		return backend;
	}

	public GProMWrapper getW() {
		return w;
	}

	public void setW(GProMWrapper w) {
		this.w = w;
	}

	public Connection getWrappedCon () {
		return this.con;
	}
	
//	public void setBackend(BackendType backend) {
//		this.backend = backend;
//	}

	
/* NEW Methods -----------------------------------------------------------
 	public Array createArrayOf(String typeName, Object[] elements)
			throws SQLException {
		return con.createArrayOf(typeName,elements);
	}

	public Blob createBlob() throws SQLException {
		return con.createBlob();
	}

	public Clob createClob() throws SQLException {
		return con.createClob();
	}

	public NClob createNClob() throws SQLException {
		return con.createNClob();
	}

	public SQLXML createSQLXML() throws SQLException {
		return con.createSQLXML();
	}
 	public Struct createStruct(String typeName, Object[] attributes)
			throws SQLException {
		return con.createStruct(typeName, attributes);
	}
 	
 	public Properties getClientInfo() throws SQLException {
		return con.getClientInfo();
	}

	public String getClientInfo(String name) throws SQLException {
		return con.getClientInfo(name);
	}
 	public boolean isValid(int timeout) throws SQLException {
		return con.isValid(timeout);
	}
 	public void setClientInfo(Properties properties)
			throws SQLClientInfoException {
		con.setClientInfo(properties);
	}

	public void setClientInfo(String name, String value)
			throws SQLClientInfoException {
		con.setClientInfo(name, value);
	}
 
 
 	public boolean isWrapperFor(Class<?> iface) throws SQLException {
		return con.isWrapperFor(iface);
	}

	public <T> T unwrap(Class<T> iface) throws SQLException {
		return con.unwrap(iface);
	}
 
 */
}
