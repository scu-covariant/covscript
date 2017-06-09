#pragma once
#include "./runtime.hpp"
namespace cov_basic {
	void opt_expr(cov::tree<token_base*>&,cov::tree<token_base*>::iterator);
	bool optimizable(const cov::tree<token_base*>::iterator& it)
	{
		if(!it.usable())
			return false;
		token_base* token=it.data();
		if(token==nullptr)
			return true;
		switch(token->get_type()) {
		case token_types::value:
			return true;
			break;
		}
		return false;
	}
	void opt_expr(cov::tree<token_base*>& tree,cov::tree<token_base*>::iterator it)
	{
		if(!it.usable())
			return;
		token_base* token=it.data();
		if(token==nullptr)
			return;
		switch(token->get_type()) {
		case token_types::id: {
			const std::string& id=dynamic_cast<token_id*>(token)->get_id();
			if(runtime->storage.var_exsist(id)&&runtime->storage.get_var(id).is_constant())
				it.data()=new token_value(runtime->storage.get_var(id));
			return;
			break;
		}
		case token_types::expr: {
			cov::tree<token_base*>& t=dynamic_cast<token_expr*>(it.data())->get_tree();
			optimize_expression(t);
			if(optimizable(t.root())) {
				it.data()=t.root().data();
			}
			return;
			break;
		}
		case token_types::array: {
			bool is_optimizable=true;
			for(auto& tree:dynamic_cast<token_array*>(token)->get_array()) {
				optimize_expression(tree);
				if(is_optimizable&&!optimizable(tree.root()))
					is_optimizable=false;
			}
			if(is_optimizable) {
				array arr;
				bool is_map=true;
				token_value* t=nullptr;
				for(auto& tree:dynamic_cast<token_array*>(token)->get_array()) {
					const cov::any& val=parse_expr(tree.root());
					if(is_map&&val.type()!=typeid(pair))
						is_map=false;
					arr.push_back(copy(val));
				}
				if(arr.empty())
					is_map=false;
				if(is_map) {
					hash_map map;
					for(auto& it:arr) {
						pair& p=it.val<pair>(true);
						if(map.count(p.first)==0)
							map.emplace(p.first,p.second);
						else
							map.at(p.first)=p.second;
					}
					t=new token_value(cov::any::make<hash_map>(std::move(map)));
				}
				else
					t=new token_value(cov::any::make<array>(std::move(arr)));
				it.data()=t;
			}
			return;
			break;
		}
		case token_types::arglist: {
			for(auto& tree:dynamic_cast<token_arglist*>(token)->get_arglist())
				optimize_expression(tree);
			return;
			break;
		}
		case token_types::signal: {
			switch(dynamic_cast<token_signal*>(token)->get_signal()) {
			case signal_types::dot_: {
				opt_expr(tree,it.left());
				opt_expr(tree,it.right());
				token_base* lptr=it.left().data();
				token_base* rptr=it.right().data();
				if(lptr!=nullptr&&lptr->get_type()==token_types::value&&dynamic_cast<token_value*>(lptr)->get_value().type()==typeid(extension_t)&&rptr!=nullptr&&rptr->get_type()==token_types::id)
					it.data()=new token_value(dynamic_cast<token_value*>(lptr)->get_value().val<extension_t>(true)->get_var(dynamic_cast<token_id*>(rptr)->get_id()));
				return;
				break;
			}
			}
		}
		}
		opt_expr(tree,it.left());
		opt_expr(tree,it.right());
		if(optimizable(it.left())&&optimizable(it.right())) {
			token_value* token=new token_value(parse_expr(it));
			tree.erase_left(it);
			tree.erase_right(it);
			it.data()=token;
		}
	}
	void optimize_expression(cov::tree<token_base*>& tree)
	{
		opt_expr(tree,tree.root());
	}
}
