package org.gprom.jdbc.testing;


import org.gprom.jdbc.pawd.*;
import org.gprom.jdbc.pawd.Operation.Materialization;
import org.gprom.jdbc.pawd.Operation.OpType;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

//import org.gprom.jdbc.pawd.JDBCConnect;

public class VersionGraphCompose {
	private VersionGraph Graph1 ;
	private Node T;
	private final Map <Node, Materialization> MaterializationPlan = new HashMap<Node, Materialization>();
	@Before
	public void setUp() throws Exception {
		Graph1 = new VersionGraph();
		Node R = new Node(Graph1.nodeIDGenerator(),false, "R");
		Node S = new Node(Graph1.nodeIDGenerator(),false, "S");
		T = new Node(Graph1.nodeIDGenerator(),false,"T");
		Node J = new Node(Graph1.nodeIDGenerator(),false,"J");
		//construct different arraylist for Edge construction
		ArrayList<Node> NodeSetAll = new ArrayList<>(Arrays.asList(J,R,S,T));
		ArrayList<Node> startNodes = new ArrayList<>(Arrays.asList(R,J));
		ArrayList<Node> endNodes = new ArrayList<>(Arrays.asList(T));
		//construct sample operations for edge creation
		Operation op1 = new Operation("SELECT a, b * 2 AS b FROM $$1$$ ",OpType.Query);
//		Operation op2 = new Operation("SELECT sum(a), b FROM $$1$$ GROUP BY b",OpType.Update);
//		Operation op3 = new Operation ("SELECT b FROM $$1$$",OpType.Query);
		Operation op2 = new Operation("SELECT sum(a), b FROM ($$1$$ NATURAL JOIN $$2$$) GROUP BY b",OpType.Query);
		//sample set of edges
		Edge edge2 = new Edge(startNodes,endNodes,op2);
		Edge edge1 = new Edge(R,S, op1);
//		Edge edge2 = new Edge(S,T,op2);
//		Edge edge3 = new Edge(T,J,op3);
		//construct Arraylist of EDGES
		ArrayList<Edge> EdgeSetAll = new ArrayList<>( Arrays.asList(edge1,edge2));//edge3,edge4,edge5,edge6));
		//System.out.println(EdgeSetAll);
		//construct arraylist of versionedges
		ArrayList<VersionEdge> VersionEdgeSetAll = new ArrayList<VersionEdge>();
		Graph1.setEdges(EdgeSetAll);
		Graph1.setVersionEdges(VersionEdgeSetAll);
		Graph1.setNodes(NodeSetAll);
		Graph1.Configure();
		//create a version graph



		//setting up materialization plan
		//System.out.println(Graph1);
		MaterializationPlan.put(R,Materialization.isMaterialized );
		MaterializationPlan.put(S,Materialization.notMaterialized );
		MaterializationPlan.put(T,Materialization.isMaterialized);
		MaterializationPlan.put(J,Materialization.isMaterialized);
	}
	@After
	public void tearDown() throws Exception {
		JDBCConnect mine= new JDBCConnect();
		for (Map.Entry<Node, Materialization> entry : MaterializationPlan.entrySet()) {
			  if (entry.getValue().equals(Materialization.isMaterialized)) {
				  String tblName = entry.getKey().getDescription();
				  System.out.println("Deleting "+tblName);
				  mine.RunUpdate("DROP TABLE REL_"+ tblName );
			  }
			}
	}

	@Test
	public void test() {
		VersionGraphManager myManager = new VersionGraphManager();
		Graph1.Configure();
		String q = myManager.Compose(Graph1, T,MaterializationPlan);
		System.out.println(q);
		JDBCConnect mine= new JDBCConnect();
		mine.RunQuery("Select * FROM ("+q+")");
	}

}
