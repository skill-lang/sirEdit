#pragma once
#include <functional>
#include <list>
#include <string>
#include <unordered_set>
#include <sirEdit/main.hpp>


namespace sirEdit::data {
	/**
	 * Manage a include of a skill specification
	 */
	class SKilL_Include {
		private:
			std::string __path;                      /// Folder to load
			std::unordered_set<std::string> __files; /// List of the files

			/**
			 * Parse a skill source file for includes
			 * @param source the source code that should be parsed
			 * @return list of includes of the source code
			 */
			std::list<std::string> __addIncludes(const std::string& source);
			/**
			 * Add a file with following content
			 * @param file the target file
			 * @param content the file content
			 */
			void __addFile(const std::string& file, const std::string& content);
			/**
			 * Deletes a file
			 * @param file The file to delete
			 */
			void __deleteFile(const std::string& file);
		public:
			/**
			 * Creates a new skill file importer
			 * @param path the tmp path to save the specification
			 */
			SKilL_Include(std::string path) : __path(std::move(path)) {}
			/**
			 * Delete old files
			 */
			~SKilL_Include() {
				for(auto& i : this->__files)
					this->__deleteFile(i);
			}

			/**
			 * Parse a specification
			 * @param sepc The specification file to parse
			 * @param getFileFunc load and return the file content
			 */
			void parse(const std::string& spec, std::function<std::string(std::string)> getFileFunc) {
				std::list<std::string> todo = {spec};
				while(todo.size() > 0) {
					auto file = todo.front();
					todo.pop_front();
					if(this->__files.count(file) > 0)
						continue;
					auto content = getFileFunc(file);
					this->__addFile(file, content);
					this->__files.insert(file);
					auto tmp = this->__addIncludes(content);
					for(auto& i : tmp)
						todo.push_back(std::move(i));
				}
			}

			/**
			 * Generate sir representation
			 * Default filename is "spec.sir"
			 * @param spec The spec file name
			 * @return conversion was successfull
			 */
			bool convert(std::string file) {
				return sirEdit::runCodegen({"spec.sir", "--from-spec", file}, this->__path) == 0;
			}
	};
}
