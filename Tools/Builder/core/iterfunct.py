def iter_function(source):
    """ convert bazel script to simple functions, however this will not
        perfect since we never support fully structure of bazel, it's just
        a simple parser of bazel script
    """
    is_scr = False
    is_str = False
    is_dsp = False
    begin = 0
    tokens = []
    marks = []

    # @NOTE: can nhac viec viet lai ham nay bang C/D/C++
    for current, c in enumerate(source):
        if is_scr is True:
            if c == '\"' and source[current:current + 3] == '\"\"\"':
                is_scr = False
                tokens.append(('bash', source[begin + 3:current]))
                begin = current + 3
        elif is_dsp is True:
            if c == '\n':
                is_dsp = False
                begin = current + 1
            else:
                continue
        elif is_str is True:
            if c == '\"':
                tokens.append(('string', source[begin:current]))
                begin = current + 1
                is_str = False
        elif c in ['=', ',', '\n', '#']:
            if c == '=':
                tokens.append(('variable', source[begin:current]))
            elif c == '#':
                is_dsp = True
                continue
            elif c == ',':
                if current - begin > 1 and len(source[begin:current].strip()) > 0:
                    is_okey = False
                    try:
                        value = int(source[begin:current])
                        is_okey = True
                    except ValueError:
                        pass
                    try:
                        if is_okey is False:
                            value = float(source[begin:current])
                        is_okey = True
                    except ValueError:
                        pass
                    try:
                        if is_okey is False:
                            value = bool(source[begin:current])
                        is_okey = True
                    except ValueError:
                        pass
                    tokens.append(('value', source[begin:current]))
            begin = current + 1
        elif c in ['[', '(', '{', '\"', ']', ')', '}']:
            if c == '{':
                tokens.append('dict')
            elif c == '(':
                tokens.append(('function', source[begin:current]))
            elif c == '[':
                tokens.append('list')

            if c in [']', ')', '}', '\"']:
                if c == '}' and marks[-1] == '{':
                    tokens.append('enddict')
                elif c == ']' and marks[-1] == '[':
                    tokens.append('endlist')
                elif c == '\"' and marks[-1] == '\"':
                    pass
                elif c == ')' and marks[-1] == '(':
                    tokens.append('endfunction')
                elif c == '\"' and is_str is False:
                    if source[current:current + 3] == '\"\"\"':
                        is_scr = True
                    else:
                        is_str = True
                    begin = current + 1
                    continue
                else:
                    raise StandardError('')

                marks.pop()
            else:
                marks.append(c)
            begin = current + 1

    # @NOTE: sau khi lay duoc cac token, chung ta chuyen token thanh
    # list cac function
    bundle_of_functions = None
    current_variable = None
    current_dict = None
    current_list = None
    function = None
    status = 0

    for token in tokens:
        if isinstance(token, tuple):
            if token[0] == 'function':
                # @NOTE: neu function ben trong 1 function thi no can phai duoc
                # parse truoc va load vao ben trong function cha

                if status > 0 and not function is None:
                    bundle_of_functions.append({
                        'current_variable': current_variable,
                        'current_dict': current_dict,
                        'current_list': current_list,
                        'function': function
                    })
                elif status <= 0 or bundle_of_functions == None:
                    bundle_of_functions = []

                current_variable = None
                current_dict = None
                current_list = None
                status += 1
                function = {
                    'function': token[1].strip(),
                    'variables': []
                }
            elif token[0] == 'variable':
                function['variables'].append({token[1].strip(): ''})
                current_variable = len(function['variables']) - 1
            elif token[0] in ['bash', 'string', 'value']:
                if token[0] in ['string', 'value']:
                    packed = token[1]
                else:
                    packed = token

                if not current_list is None:
                    current_list.append(packed)
                elif not current_dict is None:
                    current_dict.append(packed)
                elif current_variable is None:
                    function['variables'].append(packed)
                else:
                    variable_name = list(function['variables'][current_variable].keys())[0]
                    function['variables'][current_variable][variable_name] = packed
                    current_variable = None
        elif token == 'endfunction':
            # @NOTE: chung ta da toi diem ket thuc cua 1 function, can kiem tra xem
            # cac function da duoc load hoan tat chua

            if not bundle_of_functions is None and status > 1:
                # @NOTE: function nay duoc goi boi mot function khac
                packed = function

                current_variable = bundle_of_functions[-1]['current_variable']
                current_dict = bundle_of_functions[-1]['current_dict']
                current_list = bundle_of_functions[-1]['current_list']
                function = bundle_of_functions[-1]['function']

                if packed['function'] == function:
                    bundle_of_functions.pop()

                if not current_list is None:
                    current_list.append(packed)
                elif not current_dict is None:
                    current_dict.append(packed)
                elif current_variable is None:
                    function['variables'].append(packed)
                else:
                    variable_name = list(function['variables'][current_variable].keys())[0]
                    function['variables'][current_variable][variable_name] = packed

                if len(bundle_of_functions) == 0:
                    bundle_of_functions = None

            status -= 1
            if status == 0:
                bundle_of_functions = None
                yield function
        elif token == 'list':
            current_list = []
        elif token == 'dict':
            current_dict = []
        elif token == 'endlist':
            if current_variable is None:
                function['variables'].append(current_list)
            else:
                variable_name = list(function['variables'][current_variable].keys())[0]
                function['variables'][current_variable][variable_name] = current_list
                current_variable = None
            current_list = None
        elif token == 'enddict':
            if current_variable is None:
                function['variables'].append(current_dict)
            else:
                variable_name = list(function['variables'][current_variable].keys())[0]
                function['variables'][current_variable][variable_name] = current_dict
                current_variable = None
            current_list = None
