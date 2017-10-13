package org.gprom.jdbc.driver;

import java.sql.Connection;
import java.sql.Driver;
import java.sql.DriverManager;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.Properties;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.gprom.jdbc.backends.BackendInfo;
import org.gprom.jdbc.driver.GProMJDBCUtil.BackendType;
import org.gprom.jdbc.jna.GProMJavaInterface.ConnectionParam;
import org.gprom.jdbc.jna.GProMMetadataLookupPlugin;
import org.gprom.jdbc.jna.GProMWrapper;
import org.gprom.jdbc.metadata_lookup.oracle.OracleMetadataLookup;
import org.gprom.jdbc.metadata_lookup.postgres.PostgresMetadataLookup;
import org.gprom.jdbc.metadata_lookup.sqlite.SQLiteMetadataLookup;
import org.gprom.jdbc.utility.LoggerUtil;
import org.gprom.jdbc.utility.PropertyWrapper;

/**
 * GProMDriver extends the SQL driver for adding a perm assistance
 * 
 * @author lordpretzel
 * 
 */
public class GProMDriver implements Driver {
	/** logger */
	private static Logger log = LogManager.getLogger(GProMDriver.class);
	
	
	protected Driver driver;
	private GProMWrapper w;
	
	/*
	private static final int MAJOR_VERSION = 0;
	private static final int MINOR_VERSION = 8;
	private static final String VERSION_FLAG = "b";
	private static final int CURRENT_VERSION = 1;
	 */
	public GProMDriver() {
		w = GProMWrapper.inst;
	}

	public boolean acceptsURL(String url) throws SQLException {
		if (url == null)
	        return false;
	    if (!url.startsWith("jdbc:gprom:"))
	        return false;
	    return true;

	}

	public GProMConnectionInterface connect (String url, Properties info)
			throws SQLException {
		// if the given URL could no be handled by the driver return null.
		if (!acceptsURL(url))
			return null;

		/** should we load the backend driver or not */
		boolean loadBackendDriver = Boolean.parseBoolean(GProMDriverProperties.getValueOrDefault(
				GProMDriverProperties.LOAD_DRIVER, info)); 
				
		/** should we use a Java JDBC based metadata lookup plugin */		
		boolean useJDBCMetadataLookup = Boolean.parseBoolean(GProMDriverProperties.getValueOrDefault(
				GProMDriverProperties.JDBC_METADATA_LOOKUP, info));
		
		/*
		 * Load the driver to connect to the database and create a new
		 * GProMConnection.
		 */
		try {
			w.init();
			
			BackendType backend = GProMJDBCUtil.inst.getBackendTypeFromURL(url);
			String driverClass = GProMJDBCUtil.inst.getDriverClass(backend);
			PropertyWrapper backendOpts = GProMJDBCUtil.inst.getOptionsForBackend(backend);
			Connection backendConnection;
			String backendURL = GProMJDBCUtil.inst.stripGProMPrefix(url);
			
			if (driverClass == null)
				throw new Exception("did not find driver for: " +  backend);
			
			// look if a suitable driver is in the DriverMapping if automatic loading is active
			if (loadBackendDriver && driverClass != null) {
				// load the driver from the classpath in the DriverMapping
				// property
				Class.forName(driverClass);				
			}
			
			// init a new GProMConnection from the driver loaded before and
			// return it
			driver = DriverManager.getDriver(backendURL);
			if (driver == null)
				throw new Exception("did not find class for driver: " +  driver);
			log.info("driver class is: " + driver.getClass());
			
			// create a jdbc connection to the backend.
			log.info("trying to connect to: " + backendURL);
			backendConnection = driver.connect(backendURL, info);
			if (backendConnection == null)
				throw new Exception("was unable to create connection: " + backendURL);
			log.info("created connection object: " + backendURL);
			
			// extract backend connection parameters from JDBC connection
			extractConnectionParameters(backendURL, backendOpts, backend, info);
			java.util.Iterator<java.util.HashMap.Entry<Object, Object>> infoIter = info.entrySet().iterator();
			java.util.HashMap.Entry<Object, Object> propEntry;
			while(infoIter.hasNext()){
				propEntry = infoIter.next();
				if(propEntry.getKey() instanceof String){
					if(((String)propEntry.getKey()).startsWith("plugin.") || ((String)propEntry.getKey()).startsWith("log.")){
						backendOpts.put(propEntry.getKey(), propEntry.getValue());
					}
				}
			}
			
			// setup GProM C libraries options and plugins
			w.setupOptions(backendOpts);
			if (useJDBCMetadataLookup) {
				w.setupPlugins(backendConnection, getMetadataLookup(backendConnection, backend));
			}
			else {
				w.setupPlugins();
			}
			//w.setLogLevel(4);
			log.debug("have setup options: " + w.toString());
			
			return new GProMConnection(backendConnection,
					backendOpts, backend, w);
		} catch (Throwable ex) {
			log.error("Error loading the driver and getting a connection.");
			LoggerUtil.logException(ex, log);
			System.exit(2);
		}
		return null;
	}
	
	private GProMMetadataLookupPlugin getMetadataLookup (Connection con, BackendType backend) throws SQLException {
		switch (backend)
		{
		case HSQL:
			break;
		case Oracle:
			return new OracleMetadataLookup(con).getPlugin();
		case Postgres:
			return new PostgresMetadataLookup(con).getPlugin();
		case SQLite:
			return new SQLiteMetadataLookup(con).getPlugin();
		default:
			throw new SQLException("no JDBC metadata lookup for Backend " + backend.toString());
		}
		return null;
	}
	
	/**
	 * @param backendConnection
	 * @param backendOpts
	 * @throws Exception 
	 */
	private void extractConnectionParameters(String url,
			PropertyWrapper opts, BackendType backend, Properties userOpts) throws Exception {
		BackendInfo i = GProMJDBCUtil.inst.getBackendInfo(backend);
		String db = i.getDatabase(url);
		String host = i.getHost(url);
		String user = i.getUser(url);
		String passwd = i.getPassword(url);
		String port = i.getPort(url);
		
		// try to user from info
		if (user == null) {
			if (userOpts.getProperty("user") != null)
				user = userOpts.getProperty("user");
		}
		// try to get passwd from info
		if (passwd == null) {
			if (userOpts.getProperty("password") != null)
				passwd = userOpts.getProperty("password");	
		}
		
		if (db != null)
			GProMWrapper.inst.setConnectionOption(opts, ConnectionParam.Database, db);
		if (host != null)
			GProMWrapper.inst.setConnectionOption(opts, ConnectionParam.Host, host);
		if (user != null)
			GProMWrapper.inst.setConnectionOption(opts, ConnectionParam.User, user);
		if (passwd != null)
			GProMWrapper.inst.setConnectionOption(opts, ConnectionParam.Password, passwd);
		if (port != null)
			GProMWrapper.inst.setConnectionOption(opts, ConnectionParam.Port, port);
	}

	public DriverPropertyInfo[] getPropertyInfo(String url, Properties info)
			throws SQLException {
		if (driver == null)
			return null;
		return driver.getPropertyInfo(url, info);
	}

	/**
	 * Simply through these calls to the backend driver.
	 * 
	 * @return whatever the backend driver tells us
	 */
	public int getMajorVersion() {
		if (driver == null)
			return 0;
		return driver.getMajorVersion();
	}

	public int getMinorVersion() {
		if (driver == null)
			return 0;
		return driver.getMinorVersion();
	}

	public boolean jdbcCompliant() {
		if (driver == null)
			return false;
		return driver.jdbcCompliant();
	}

	static {
		try {
			// Register this with the DriverManager
			DriverManager.registerDriver(new GProMDriver());
		} catch (SQLException e) {
			e.printStackTrace();
			System.exit(-1);
		}
	}

	/* (non-Javadoc)
	 * @see java.sql.Driver#getParentLogger()
	 */
	@Override
	public java.util.logging.Logger getParentLogger()
			throws SQLFeatureNotSupportedException {
		// TODO Auto-generated method stub
		return null;
	}

}
