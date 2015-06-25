package org.gprom.jdbc.test.testgenerator;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.sql.SQLException;
import java.util.InvalidPropertiesFormatException;
import java.util.Map.Entry;
import java.util.Properties;

import org.gprom.jdbc.driver.GProMConnection;
import org. gprom.jdbc.test.testgenerator.dataset.DataAndQueryGenerator;
import org.apache.log4j.Logger;
import org.dbunit.Assertion;
import org.dbunit.DBTestCase;
import org.dbunit.PropertiesBasedJdbcDatabaseTester;
import org.dbunit.database.IDatabaseConnection;
import org.dbunit.dataset.IDataSet;
import org.dbunit.dataset.ITable;
import org.dbunit.dataset.xml.XmlDataSet;
import org.dbunit.operation.DatabaseOperation;

public class AbstractGProMTester extends DBTestCase {

	static Logger log = Logger.getLogger(AbstractGProMTester.class);
	
	protected String path;
	protected Properties oldProps;
	
	public AbstractGProMTester (String name) {
		super(name);
	}
	
	protected void setUp () throws Exception {
		super.setUp();
		setDBTestCaseProps();
	}

	protected void setDBTestCaseProps () {
		try {
			System.setProperty( PropertiesBasedJdbcDatabaseTester.DBUNIT_DRIVER_CLASS, "org.gprom.jdbc.driver.GProMDriver" );
	        System.setProperty( PropertiesBasedJdbcDatabaseTester.DBUNIT_CONNECTION_URL, "jdbc:gprom:postgresql://127.0.0.1:5432/" + ConnectionOptions.getInstance().getDbName());
	        System.setProperty( PropertiesBasedJdbcDatabaseTester.DBUNIT_USERNAME, ConnectionOptions.getInstance().getUser());
	        System.setProperty( PropertiesBasedJdbcDatabaseTester.DBUNIT_PASSWORD, ConnectionOptions.getInstance().getPassword() );
			System.setProperty( PropertiesBasedJdbcDatabaseTester.DBUNIT_SCHEMA, ConnectionOptions.getInstance().getSchema() );
		}
		catch (Exception e) {
			System.err.println(e.toString());
		}
	}
	
    protected void tearDown() throws Exception {
    	resetOptions();
    	super.tearDown();
    }
	
	/**
	 * @throws Exception 
	 * @throws NumberFormatException 
	 * 
	 */
	protected void resetOptions() throws NumberFormatException, Exception {
		if (oldProps != null) {
			GProMConnection g = ConnectionManager.getInstance().getGProMConnection();
			
			for(Entry<?, ?> e: oldProps.entrySet()) {
    			String key = (String) e.getKey();
    			String value = (String) e.getValue();
    			log.debug("set key " + key + " to " + value);
    			g.getW().setOption(key, value);
    		}
		}
	}

	/* (non-Javadoc)
	 * @see org.dbunit.DatabaseTestCase#getDataSet()
	 */
	@Override
	protected IDataSet getDataSet () throws Exception {
//		return null;
		return new XmlDataSet(new FileInputStream(new File(path + "/testdb/smallTestDB.xml")));
	}

    protected DatabaseOperation getSetUpOperation() throws Exception {
        return DatabaseOperation.NONE;
    }

    protected DatabaseOperation getTearDownOperation() throws Exception {
        return DatabaseOperation.NONE;
    }
    
    protected IDatabaseConnection getConnection() throws Exception {
    	return ConnectionManager.getInstance().getIDatabaseConnection();
    }
    
    /*
     * method to execute test 
     */
    
    protected void testSQLFile (String name) throws SQLException, Exception {
    	ITable expected;
    	ITable actualResult = null;
    	String queryString;
    	boolean markedError;
    	DataAndQueryGenerator generator;
    	Properties options;
    	
    	TestInfoHolder.getInstance().setGenerator(name);
    	generator = TestInfoHolder.getInstance().getCurrentGenerator();    	
    	
    	GProMConnection g = ConnectionManager.getInstance().getGProMConnection();
    	
    	options = TestInfoHolder.getInstance().getCurrentGenerator().getOptions();
    	
    	if (options != null)
    	{
    		oldProps = new Properties();
    		for(Entry<?, ?> e: options.entrySet()) {
    			String key = (String) e.getKey();
    			String value = (String) e.getValue();
    			oldProps.setProperty(key, g.getW().getOption(key));
    			log.debug("set key " + key + " to " + value);
    			g.getW().setOption(key, value);
    		}
    		
    		g.getW().reconfPlugins();
    	}
    	else
    		oldProps = null;
    
    	for(int i = 1; i <= generator.getNumTest(); i++) {
    		expected = generator.getExpectedResult("q" + i);
    		queryString = generator.getQuery("q" + i);
    		markedError = generator.isError("q" + i); 
    		
    		logTestResult (name, queryString, i, markedError);
    		
    		if (!markedError) {
    			actualResult = getConnection().createQueryTable("q" + i + ".result", queryString);
	    		Assertion.assertEquals(actualResult, expected);
    		}
    		
    	}
    }
    
    protected void setGenerator (String name) throws Exception {
      	DataAndQueryGenerator generator;
    	Properties options;
    	
    	System.out.println("\n********************************************");
    	System.out.println("**   " + name);
    	System.out.println("********************************************\n");
    	
    	TestInfoHolder.getInstance().setGenerator(name);
    	generator = TestInfoHolder.getInstance().getCurrentGenerator();    	
    	
    	GProMConnection g = ConnectionManager.getInstance().getGProMConnection();
    	
    	options = TestInfoHolder.getInstance().getCurrentGenerator().getOptions();
    	
    	if (options != null)
    	{
    		oldProps = new Properties();
    		for(Entry<?, ?> e: options.entrySet()) {
    			String key = (String) e.getKey();
    			String value = (String) e.getValue();
    			oldProps.setProperty(key, g.getW().getOption(key));
    			log.debug("set key " + key + " to " + value);
    			g.getW().setOption(key, value);
    		}
    		
    		g.getW().reconfPlugins();
    	}
    	else
    		oldProps = null;
    }
    
    protected void testSingleQuery (int num) throws Exception {
    	ITable[] expecteds;
    	ITable actualResult = null;
    	String queryString;
    	boolean markedError;
    	DataAndQueryGenerator generator;
    	
    	generator = TestInfoHolder.getInstance().getCurrentGenerator();
    	
    	expecteds = generator.getExpectedResults("q" + num);
		queryString = generator.getQuery("q" + num);
		markedError = generator.isError("q" + num); 
		
		logTestResult (TestInfoHolder.getInstance().getGeneratorName() + "\t", queryString, num, markedError);
		
		try {
			if (!markedError) {
				actualResult = getConnection().createQueryTable("q" + num + ".result", queryString);
	    		Assertion.assertEquals(expecteds, actualResult, queryString);
			}
		}
		catch (Exception e)
		{
			System.out.println("QUERY: " + queryString + "\n\n");
			throw new Exception(queryString, e);
		}
    }
    

    private void logTestResult (String name, String query, int numTest, boolean markedError) {
    	if (markedError) {
    		System.out.println(name + " - " + numTest + ": ERROR MARKED ");
    		TestInfoHolder.getInstance().addMarkedError(name + " - " + numTest + ": ERROR MARKED ");
    	}
    	else {
    		System.out.println(name + " - " + numTest + ": TRY");
    	}
    }

}
