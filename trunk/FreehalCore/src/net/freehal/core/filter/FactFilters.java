package net.freehal.core.filter;

import java.util.ArrayList;
import java.util.List;

import net.freehal.core.xml.XmlFact;

public class FactFilters implements FactFilter {
	
	static List<FactFilter> factfilters = null;
	
	static synchronized void addFilter(FactFilter filter) {
		if (factfilters == null) {
			factfilters = new ArrayList<FactFilter>();
		}
		factfilters.add(filter);
	}

	@Override
	public double filter(XmlFact f1, XmlFact f2, double match) {
		for (FactFilter filter : factfilters) {
			match = filter.filter(f1, f2, match);
		}
		return match;
	}
}
