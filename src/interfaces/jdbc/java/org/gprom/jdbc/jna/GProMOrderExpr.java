package org.gprom.jdbc.jna;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;
/**
 * This file was autogenerated by <a href="http://jnaerator.googlecode.com/">JNAerator</a>,<br>
 * a tool written by <a href="http://ochafik.com/">Olivier Chafik</a> that <a href="http://code.google.com/p/jnaerator/wiki/CreditsAndLicense">uses a few opensource projects.</a>.<br>
 * For help, please visit <a href="http://nativelibs4java.googlecode.com/">NativeLibs4Java</a> , <a href="http://rococoa.dev.java.net/">Rococoa</a>, or <a href="http://jna.dev.java.net/">JNA</a>.
 */
public class GProMOrderExpr extends GProMStructure {
	public int type;
	/** C type : GProMNode* */
	public org.gprom.jdbc.jna.GProMNode.ByReference expr;
	/**
	 * @see GProMSortOrder<br>
	 * C type : GProMSortOrder
	 */
	public int order;
	/**
	 * @see GProMSortNullOrder<br>
	 * C type : GProMSortNullOrder
	 */
	public int nullOrder;
	public GProMOrderExpr() {
		super();
	}
	public GProMOrderExpr(Pointer address) {
		super(address);
	}
	protected List<? > getFieldOrder() {
		return Arrays.asList("type", "expr", "order", "nullOrder");
	}
	/**
	 * @param expr C type : GProMNode*<br>
	 * @param order @see GProMSortOrder<br>
	 * C type : GProMSortOrder<br>
	 * @param nullOrder @see GProMSortNullOrder<br>
	 * C type : GProMSortNullOrder
	 */
	public GProMOrderExpr(int type, org.gprom.jdbc.jna.GProMNode.ByReference expr, int order, int nullOrder) {
		super();
		this.type = type;
		this.expr = expr;
		this.order = order;
		this.nullOrder = nullOrder;
	}
	public static class ByReference extends GProMOrderExpr implements Structure.ByReference {
		
	};
	public static class ByValue extends GProMOrderExpr implements Structure.ByValue {
		
	};
}
