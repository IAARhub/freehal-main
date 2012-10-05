/*******************************************************************************
 * Copyright (c) 2006 - 2012 Tobias Schulz and Contributors.
 * 
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 ******************************************************************************/
package net.freehal.app.impl;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

import net.freehal.app.util.ExecuteLater;
import net.freehal.app.util.Util;
import net.freehal.compat.android.AndroidCompatibility;
import net.freehal.compat.android.SqliteFreehalFile;
import net.freehal.compat.android.AndroidLogUtils;
import net.freehal.compat.sunjava.StandardFreehalFile;
import net.freehal.compat.sunjava.StandardLogUtils;
import net.freehal.core.answer.AnswerProvider;
import net.freehal.core.answer.AnswerProviders;
import net.freehal.core.database.Database;
import net.freehal.core.database.DatabaseAnswerProvider;
import net.freehal.core.database.DiskDatabase;
import net.freehal.core.database.DiskStorage;
import net.freehal.core.filter.FactFilters;
import net.freehal.core.filter.FilterNoNames;
import net.freehal.core.filter.FilterNot;
import net.freehal.core.filter.FilterQuestionExtra;
import net.freehal.core.filter.FilterQuestionWhat;
import net.freehal.core.filter.FilterQuestionWho;
import net.freehal.core.grammar.Grammar;
import net.freehal.core.grammar.Grammars;
import net.freehal.core.lang.Languages;
import net.freehal.core.lang.english.EnglishGrammar;
import net.freehal.core.lang.english.EnglishParser;
import net.freehal.core.lang.english.EnglishPredefinedAnswerProvider;
import net.freehal.core.lang.english.EnglishTagger;
import net.freehal.core.lang.english.EnglishWording;
import net.freehal.core.lang.german.GermanGrammar;
import net.freehal.core.lang.german.GermanParser;
import net.freehal.core.lang.german.GermanPredefinedAnswerProvider;
import net.freehal.core.lang.german.GermanRandomAnswerProvider;
import net.freehal.core.lang.german.GermanTagger;
import net.freehal.core.lang.german.GermanWording;
import net.freehal.core.parser.Parser;
import net.freehal.core.parser.Sentence;
import net.freehal.core.pos.Tagger;
import net.freehal.core.pos.Taggers;
import net.freehal.core.pos.storage.TaggerCache;
import net.freehal.core.pos.storage.TaggerCacheDisk;
import net.freehal.core.storage.StandardStorage;
import net.freehal.core.storage.Storages;
import net.freehal.core.util.FreehalFiles;
import net.freehal.core.util.LogUtils;
import net.freehal.core.util.StringUtils;
import net.freehal.core.wording.Wording;
import net.freehal.core.wording.Wordings;

public class FreehalImplOffline extends FreehalImpl {

	private static FreehalImplOffline instance;

	private File centralLogFile = null;

	private String input;
	private String output;

	private FreehalImplOffline() {
		init();
	}

	private void init() {

		// android.os.Debug.startMethodTracing("init");

		// file access: use the android sqlite API for all files with
		// "sqlite://" protocol, and a normal file for all other protocols
		FreehalFiles.add(FreehalFiles.ALL_PROTOCOLS, new StandardFreehalFile(null));
		FreehalFiles.add("sqlite", new SqliteFreehalFile(null));

		// set the language and the base directory (if executed in "bin/", the
		// base directory is ".."). Freehal expects a "lang_xy" directory there
		// which contains the database files.
		Languages.setLanguage(AndroidCompatibility.getLanguage());
		Storages.setStorage(new StandardStorage(AndroidCompatibility.getPath()));

		// how and where to print the log
		StandardLogUtils log = new StandardLogUtils();
		log.to(AndroidLogUtils.AndroidLogStream.create());
		log.to(StandardLogUtils.FileLogStream
				.create(centralLogFile = Storages.inPath("stdout.txt").getFile()));
		LogUtils.set(log);

		ExecuteLater later = new ExecuteLater(0) {
			@Override
			public void run() {}

			@Override
			protected Void doInBackground(Void... params) {

				// Runtime.getRuntime().

				// unpack the zip file which contains the standard database
				Util.unpackZip(
						Util.getActivity().getResources().openRawResource(net.freehal.app.R.raw.database),
						Storages.getPath());

				final boolean isGerman = Languages.getLanguage().equals("de");

				// initialize the grammar
				// (also possible: EnglishGrammar, GermanGrammar, FakeGrammar)
				Grammar grammar = isGerman ? new GermanGrammar() : new EnglishGrammar();
				grammar.readGrammar(FreehalFiles.getFile("grammar.txt"));
				Grammars.setGrammar(grammar);

				// initialize the part of speech tagger
				// (also possible: EnglishTagger, GermanTagger, FakeTagger)
				// the parameter is either a TaggerCacheMemory (faster, higher
				// memory usage) or a TaggerCacheDisk (slower, less memory
				// usage)
				TaggerCache cache = new TaggerCacheDisk();
				Tagger tagger = isGerman ? new GermanTagger(cache) : new EnglishTagger(cache);
				tagger.readTagsFrom(FreehalFiles.getFile("guessed.pos"));
				tagger.readTagsFrom(FreehalFiles.getFile("brain.pos"));
				tagger.readTagsFrom(FreehalFiles.getFile("memory.pos"));
				tagger.readRegexFrom(FreehalFiles.getFile("regex.pos"));
				tagger.readToggleWordsFrom(FreehalFiles.getFile("toggle.csv"));
				Taggers.setTagger(tagger);

				// how to phrase the output sentences
				// (also possible: EnglishPhrase, GermanPhrase, FakePhrase)
				Wording phrase = isGerman ? new GermanWording() : new EnglishWording();
				Wordings.setWording(phrase);

				// initialize the database
				// (also possible: DiskDatabase, FakeDatabase)
				Database database = new DiskDatabase();
				// set the maximum amount of facts to cache
				DiskDatabase.setMemoryLimit(2500);
				DiskStorage.Key.setGlobalKeyLength(2);

				// while updating the cache, a cache_xy/ directory will be
				// filled with information from the database files in lang_xy/
				database.updateCache();

				// Freehal has different ways to find an answer for an input
				AnswerProviders
						.getInstance()
						.add(isGerman ? new GermanPredefinedAnswerProvider()
								: new EnglishPredefinedAnswerProvider())
						.add(new DatabaseAnswerProvider(database))
						.add(isGerman ? new GermanRandomAnswerProvider() : null)
						.add(new FakeAnswerProvider());

				// fact filters are used to filter the best-matching fact in the
				// database
				FactFilters.getInstance().add(new FilterNot()).add(new FilterNoNames())
						.add(new FilterQuestionWho()).add(new FilterQuestionWhat())
						.add(new FilterQuestionExtra());

				// android.os.Debug.stopMethodTracing();

				return null;
			}
		};
		later.execute();
	}

	public static FreehalImpl getInstance() {
		if (instance == null)
			instance = new FreehalImplOffline();
		return instance;
	}

	@Override
	public void setInput(String input) {
		this.input = input;
	}

	@Override
	public void compute() {
		// also possible: EnglishParser, GermanParser, FakeParser
		Parser p = Languages.getLanguage().isCode("de") ? new GermanParser(input) : new EnglishParser(input);

		// parse the input and get a list of sentences
		final List<Sentence> inputParts = p.getSentences();

		List<String> outputParts = new ArrayList<String>();
		// for each sentence...
		for (Sentence s : inputParts) {
			// get the answer using the AnswerProvider API
			outputParts.add(AnswerProviders.getInstance().getAnswer(s));
		}
		// put all answers together
		output = StringUtils.join(" ", outputParts);
		LogUtils.i("Input: " + input);
		LogUtils.i("Output: " + output);
	}

	@Override
	public String getOutput() {
		return output;
	}

	@Override
	public String getLog() {
		try {
			return new Scanner(centralLogFile).useDelimiter("\\Z").next();
		} catch (FileNotFoundException e) {
			LogUtils.e(e);
			return StringUtils.asString(e);
		}
	}

	@Override
	public String getGraph() {
		return null;
	}

	@Override
	public String getVersionName() {
		return "not installed";
	}

	@Override
	public int getVersionCode() {
		return -1;
	}
}

class FakeAnswerProvider implements AnswerProvider {

	@Override
	public String getAnswer(Sentence s) {
		return "Hello World!";
	}

}
