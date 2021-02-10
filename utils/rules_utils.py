from yaml import Loader, load
from utils.templating_utils import removeComments, removeBlankLines, TemplateSubst

def parse_rules(rules_payload):
    rules_obj = load(rules_payload, Loader=Loader)
    return rules_obj

def subst_rules(rules, registers, template_payload):
    template_payload = removeComments(template_payload)
    template_payload = removeBlankLines(template_payload)
    ts = TemplateSubst(rules,registers)
    template_payload = ts.substitute_values(template_payload)
    return template_payload