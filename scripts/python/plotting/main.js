
	const gui = {};
	gui.perf_proc_active = false;
	gui.test_proc_active = false;
	gui.fit_proc_active = false;

	const sleep = ms => new Promise(r => setTimeout(r, ms));

	async function read_json_file(filename) {
		fst = 'filename=' + filename;
		fst += '&cmd=read';
		const response = await fetch('http://localhost:8000/file?' + fst,
			{
				method: 'GET'
			}
		);
		json_data = await response.json();
		return json_data
	}

	async function write_json_file(filename, json_data) {
		fst = 'filename=' + filename;
		fst += '&cmd=write';
		fst += '&json_data=' + JSON.stringify(json_data);
		await fetch('http://localhost:8000/file?' + fst,
			{
				method: 'GET'
			}
		);
	}

	async function copy_json_file(filename, new_filename) {
		fst = 'cmd=copy';
		fst += '&filename=' + filename;
		fst += '&new_filename=' + new_filename;
		await fetch('http://localhost:8000/file?' + fst,
			{
				method: 'GET'
			}
		);
	}

	async function delete_file(filename) {
		fst = 'filename=' + filename;
		fst += '&cmd=delete';
		await fetch('http://localhost:8000/file?' + fst,
			{
				method: 'GET'
			}
		);
	}

	async function callPyServer(cmd, fst) {
		await fetch('http://localhost:8000/' + cmd + '?' + fst);
	}

	async function callPyServerJSON(cmd, fst) {
		const response = await fetch('http://localhost:8000/' + cmd + '?' + fst);
		data = await response.json();
		return data;
	}

	//
	let ws_connection = null;
	async function init_websocket() {
		await callPyServer("launch_ws", "")
		ws_connection = await new WebSocket("ws://localhost:8600");
		ws_connection.addEventListener("message", async (msg) => {
			if ('data' in msg)
			{
				const data = JSON.parse(msg['data']);
				if ('dest' in data)
					if(data['dest'] == "index")
						if ('source' in data)
						{
							if (!data['source'].localeCompare("test-proc"))
								if ('cmd' in data)
								{
									if(!data['cmd'].localeCompare("init"))									
									{
										await get_elements();
										await set_elements();
									}
									if(!data['cmd'].localeCompare("refresh"))									
									{
										//await get_elements();
										//await set_elements();
									}
								}

							if (!data['source'].localeCompare("perf-proc"))								
								if ('cmd' in data)
								{
									if(!data['cmd'].localeCompare("init"))									
									{
										await get_elements();
										await set_elements();
									}
								}

						if (!data['source'].localeCompare("fit-proc"))
							if ('cmd' in data)
							{
									if (!data['cmd'].localeCompare("refresh"))
										fill_fit_table()
							}			
				}
			}});
	}

	//
	async function init_elements() {
		params_table = document.getElementById('params_table');
		var rowCount = params_table.rows.length;
		for (var x=rowCount-1; x>0; x--) {
		   params_table.deleteRow(x);
		}
		await set_elements();
	}

	async function get_elements() {
		const model_form = document.getElementById('model_form');
		const build_form = document.getElementById('build_form');
		const test_form = document.getElementById('test_form');

		prefs = await read_json_file("./prefs.json")
		prefs["model_id"] = model_form.model_id.value;
		prefs["build_dir"] = build_form.build_dir.value;
		prefs['tests']['list'] = test_form.test_list.value;
		prefs['tests']['id'] = test_form.test_id.value;
		prefs['tests']['draw_profile'] = test_form.draw_profile.value;
		await write_json_file("./prefs.json", prefs);
	}

	async function update_models() 
	{
			var model_cache = await read_json_file("./model_cache.json");
			for (model_id in model_cache)
			{
				try
				{
					const ref_model_filepath = "../../../test/models_json/" + model_id + ".json";
					await copy_json_file(model_cache[model_id], ref_model_filepath);
					delete model_cache[model_id];

				}
				catch(err)
				{}

			}
			await write_json_file("./model_cache.json", model_cache);
	}

	async function restore_models() 
	{
			var model_cache = await read_json_file("./model_cache.json");
			for (model_id in model_cache)
			{
				try
				{
					await delete_file(model_cache[model_id]);
					delete model_cache[model_id];
				}
				catch(err)
				{}
			}
			await write_json_file("./model_cache.json", model_cache);
	}

	async function set_elements() {
		var prefs = await read_json_file("./prefs.json");

		const model_form = document.getElementById('model_form');
		const build_form = document.getElementById('build_form');
		const test_form = document.getElementById('test_form');

		// update models control
		test_form.test_list.value = prefs['tests']['list'];

		let select_model = model_form.model_id;
		while (select_model.hasChildNodes())
			select_model.removeChild(select_model.firstChild);

		var num_models = 0;
		const models = await read_json_file("./model_index.json");
		for (let model_id in models) {
				let option = document.createElement("option");
				option.text = ('name' in models[model_id])?  models[model_id]['name']: model_id;
				option.value = model_id;
				select_model.appendChild(option);
				def_model_id = model_id;
				num_models++;
		}
		if (('model_id' in prefs) && (prefs['model_id'] in models))
			def_model_id = prefs['model_id'];
		prefs['model_id'] = def_model_id;

		model_form.model_id.value = prefs['model_id'];
		select_model.disabled = (num_models == 0);

		build_form.build_dir.value = ("build_dir" in prefs) ? prefs["build_dir"] : "../../build";

		if (gui.perf_proc_active) 
		{
			const ref_model_filepath = "../../../test/models_json/" + prefs['model_id'] + ".json";
			var model_cache = await read_json_file("./model_cache.json");
			if (!(prefs['model_id'] in model_cache))
			{
				model_cache[prefs['model_id']] = prefs["build_dir"] + "/gui/" + prefs['model_id'] + ".json"
				await copy_json_file(ref_model_filepath, model_cache[prefs['model_id']]);
				await write_json_file("./model_cache.json", model_cache);
			}
			// send perf info
			var msg = {
				'source': 'index',
				'dest': 'perf-proc',
				'cmd': 'replot',
				'model_filepath': model_cache[prefs['model_id']]
			};
			try {
				await ws_connection.send(JSON.stringify(msg));
			}
			catch(err)
			{
				await init_websocket();
			}
		};

		// update select_test control
		while (test_form.test_id.hasChildNodes())
			test_form.test_id.removeChild(test_form.test_id.firstChild);

		// get available tests
		var measured_filepath = ""
		var simulated_filepath = ""
		const tests = await read_json_file("./test_index.json");
		var show_all_tests = !prefs["tests"]["list"].localeCompare("all_tests");
		var show_tests_with_data = !prefs["tests"]["list"].localeCompare("tests_with_data");
		var show_standard_tests = !prefs["tests"]["list"].localeCompare("standard_tests");
		var def_test_list = "";
		var def_test_id = "";
		var have_test = false;
		var found_measured = false;
		var num_tests = 0;
		for (let test_id in tests) {
			var show_test = show_all_tests;
			if ('group' in tests[test_id]) {
				const test_group = tests[test_id]['group'];
				show_test |= show_standard_tests && !test_group.localeCompare("standard_tests");
				if (show_all_tests || show_tests_with_data)
					if ('measured' in tests[test_id])	 // check for measured
							if (prefs['model_id'] in tests[test_id]['measured'])
							{
								show_test = true;
								found_measured = true;
							}
			}
			if (show_test)
			{
				let option = document.createElement("option");
				option.text = ('name' in tests[test_id])?  tests[test_id]['name']: test_id;
				option.value = test_id;
				test_form.test_id.appendChild(option);
				if (!have_test || (test_id == prefs['tests']['id']))
					def_test_id = test_id;
				have_test = true;
				num_tests++;
			}
		}
		prefs['tests']['id'] = def_test_id;

		const select_draw_profile = document.getElementById('draw_profile');
			
		test_form.test_list.value = prefs['tests']['list'];
		test_form.test_id.value = prefs['tests']['id'];
		select_draw_profile.value = prefs['tests']["draw_profile"];

		for (let option in test_form.test_list.children)
		{
			if (test_form.test_list.children[option].valueOf == "tests_with_data") 
				test_form.test_list.children[option].disabled = !found_measured;
		}	

		test_form.test_id.disabled = !have_test;

		let is_standard_test = false;
		let test_data = {};
		if (have_test)
		{
			test_data = tests[prefs['tests']['id']];
			is_standard_test = (('group' in test_data) && (test_data['group'] == "standard_tests"));
		}

		select_draw_profile.disabled = !(have_test && is_standard_test);

		//
		if (have_test && gui.test_proc_active)
		{
			const output_dir = prefs['build_dir'] + "/test/output";

			const ref_model_filepath = "../../../test/models_json/" + prefs['model_id'] + ".json";
			var model_cache = await read_json_file("./model_cache.json");
			if (!(prefs['model_id'] in model_cache))
			{
				model_cache[prefs['model_id']] = prefs["build_dir"] + "/gui/" + prefs['model_id'] + ".json"
				await copy_json_file(ref_model_filepath, model_cache[prefs['model_id']]);
				await write_json_file("./model_cache.json", model_cache);
			}
			model_filepath = 	model_cache[prefs['model_id']];

			if (is_standard_test)
			{
				let data = {'model_spec': 'JSON', 'model_id_or_filepath': model_filepath, 'build_dir': prefs['build_dir'], 'draw_profile': prefs['tests']["draw_profile"]};
				await callPyServer("measure", "data=" + JSON.stringify(data))
				simulated_filepath = output_dir + "/test24hrEF_" + prefs["model_id"] + ".csv";
			}
			else
			{
				const test_dir = "../../../test/" + (('path' in test_data)? test_data['path' ] + "/": "") + prefs['tests']['id'];
				let data = {'model_spec': 'JSON', 'model_id_or_filepath': model_filepath, 'build_dir': prefs['build_dir'], 'test_dir': test_dir};
				await callPyServer("simulate", "data=" + JSON.stringify(data))

				simulated_filepath = output_dir + "/" + prefs['tests']['id'] + "_JSON_" + prefs["model_id"] + ".csv";
				if ("measured" in test_data)
					for (let model_id in test_data["measured"])
						if (model_id == prefs["model_id"]) {
							measured_filepath = test_dir + "/" + test_data['measured'][model_id];
							break;
						}
			}

			var msg = {
				'source': 'index',
				'dest': 'test-proc',
				'cmd': 'replot',
				'measured_filepath': measured_filepath,
				'simulated_filepath': simulated_filepath,
				'is_standard_test': (is_standard_test ? 1 : 0)
			}
			await ws_connection.send(JSON.stringify(msg));
			test_plot.style = "display:block;";
		}
		else
		{
			test_plot.style = "display:none;";
		}

		await write_json_file("./prefs.json", prefs)
	}

	async function change_menu_value() {
		await get_elements();
		await set_elements();
	}

	async function do_keydown(e)
	{
		// send test info
			console.log(e.key)
			var msg = {'source': 'index.html', 'dest': 'perf-proc', 'cmd': 'key-pressed', 'key': e.key};
			console.log(msg)
			await ws_connection.send(JSON.stringify(msg));
	}

	async function click_test_proc() {
		document.getElementById("test_tab").innerHTML = "Test...";
		var prefs = await read_json_file("./prefs.json")
		document.getElementById("test_btn").disabled = true;
		if (gui.test_proc_active)
		{
			var data = {'cmd': 'stop'};
			await callPyServerJSON("test_proc", "data=" + JSON.stringify(data));
			document.getElementById("test_plot").src = "";
			document.getElementById("test_btn").innerHTML = "start";
			document.getElementById("test_plot").style="display:none;"
			gui.test_proc_active = false;
			document.getElementById("test_btn").disabled = false;
		}
		else
		{
			var data = {'cmd': 'start'};
			response = await callPyServerJSON("test_proc", "data=" + JSON.stringify(data));
			document.getElementById("test_plot").src = "http://localhost:" + response["port_num"];
			document.getElementById("test_plot").style = "display:block;"
			document.getElementById("test_btn").innerHTML = "stop";	
			gui.test_proc_active = true;
		}
		await set_elements();
		document.getElementById("test_btn").disabled = false;
		document.getElementById("test_tab").innerHTML = "Test";
	}

	async function click_perf_proc() {
		document.getElementById("perf_tab").innerHTML = "Performance...";
		document.getElementById("perf_btn").disabled = true;
		if (gui.perf_proc_active)
		{
			var data = {'cmd': 'stop'};
			await callPyServerJSON("perf_proc",  "data=" + JSON.stringify(data));
			document.getElementById("perf_plot").src = "";
			document.getElementById("perf_btn").innerHTML = "start";
			document.getElementById("perf_plot").style="display:none;"
			gui.perf_proc_active = false;
		}
		else
		{
			var data = {'cmd': 'start'};
			let perf_results = await callPyServerJSON("perf_proc", "data=" + JSON.stringify(data))
			const dash_port = perf_results["port_num"];

			document.getElementById("perf_btn").innerHTML = "stop";
			document.getElementById("perf_plot").src = "http://localhost:" + dash_port
			document.getElementById("perf_plot").style = "display:block;"
			gui.perf_proc_active = true;
		}
		await set_elements();
		document.getElementById("perf_tab").innerHTML = "Performance";
		document.getElementById("perf_btn").disabled = false;
	}

	async function click_fit_proc() {
		document.getElementById("fit_btn").disabled = true;
		var data = {'cmd': 'start'}
		let fit_results = await callPyServerJSON("fit_proc", "data=" + JSON.stringify(data))

		data = {'cmd': 'stop'}
		await callPyServerJSON("fit_proc", "data=" + JSON.stringify(data))
		document.getElementById("fit_btn").disabled = false;

		await set_elements();
	}

	async function clear_params() {
			var fit_list = await read_json_file("./fit_list.json")
			fit_list['parameters'] = []
			await write_json_file("./fit_list.json", fit_list)
			await fill_fit_table()
		}

	async function clear_metrics() {
			var fit_list = await read_json_file("./fit_list.json")
			fit_list['metrics'] = []
			await write_json_file("./fit_list.json", fit_list)
			await fill_fit_table()
		}

	async function fill_params_table(fit_list) {	
		let tableHTML = ''

		have_point = false;
		if ('parameters' in fit_list)
		{		
			let params = fit_list['parameters']
			params.forEach(param =>
			{
				if ('type' in param)
					if(!param['type'].localeCompare('perf-point'))
					{
						const tableHeaders = Object.keys(param);
						if (!have_point)
						{
							tableHTML = '<table><thead><tr>';
							tableHeaders.forEach(header => {
							    tableHTML += `<th>${header}</th>`;
							  });
							tableHTML += '</tr></thead><tbody>';
							have_point = true;	
						}
						
				    tableHTML += '<tr>';
				    tableHeaders.forEach(header => {
				      tableHTML += `<td>${param[header] || ''}</td>`; 
				    });
				    tableHTML += '</tr>';
				  
					}
			});
		}
		if (!have_point)
			tableHTML = '<div>No parameters.</div>'
		document.getElementById('params_table').innerHTML = tableHTML;
	}

	async function fill_metrics_table(fit_list) {
		let tableHTML = '<table>';
		if ('metrics' in fit_list)
		{		
			let metrics = fit_list['metrics']
			have_metric = false;
			if('test_points' in metrics)
			{
				const tableHeaders = ['model_id', 'test_id', 't_min', 'variable', 'value'];
				for(const test_point of metrics['test_points'])
				{
					if (!have_point)
					{
						tableHTML += '<thead><tr>';
						tableHeaders.forEach(header => {
						    tableHTML += `<th>${header}</th>`;
						  });
						tableHTML += '</tr></thead><tbody>';
						have_point = true;	
					}
						
					tableHTML += '<tr>';
					tableHeaders.forEach(header => {
						tableHTML += `<td>${test_point[header] || ''}</td>`; 
					});
					tableHTML += '</tr>';
				  have_point = true;
				};
				tableHTML += '</tbody>';
				have_metric |= have_point;
			}

			have_energy_factor = false;
			if('energy_factors' in metrics)
			{
				const tableHeaders = ['model_id', 'draw_profile', 'value'];
				for (const energy_factor of metrics['energy_factors'])
				{
					if (!have_energy_factor)
					{
						tableHTML += '<thead><tr>';
						tableHeaders.forEach(header => {
						    tableHTML += `<th>${header}</th>`;
						  });
						tableHTML += '</tr></thead><tbody>';
						have_energy_factor = true;	
					}
						
					tableHTML += '<tr>';
					tableHeaders.forEach(header => {
						tableHTML += `<td>${energy_factor[header] || ''}</td>`; 
					});
				   tableHTML += '</tr>';
				  have_energy_factor = true;
				}
				tableHTML += '</tbody>';
				have_metric |= have_energy_factor;
			}
		}
		if (!have_metric)
			tableHTML = '<div>No metrics.</div>'
		document.getElementById('metrics_table').innerHTML = tableHTML;
	}

	async function fill_fit_table() {

		var fit_list = await read_json_file("./fit_list.json");

		await fill_params_table(fit_list);
		await fill_metrics_table(fit_list);
	}

	async function fit()	{

	}

	async function fill_general_table(model_data) {
		let tableHTML = '<table>'
		
		const tableHeaders = ['parameter', 'value'];
		tableHTML += '<thead><tr>';
		tableHeaders.forEach(header => {
		    tableHTML += `<th>${header}</th>`;
		  });
		tableHTML += '</tr></thead>';

		const tableEntries = ['number_of_nodes', 'standard_setpoint'];
		tableHTML += '<tbody>';
		tableEntries.forEach(entry => {	
			if (entry in model_data)
				tableHTML += '<tr> <th>' + entry + '</th> <td>' + model_data[entry] + '</td> </tr>';
		});
		tableHTML += '</tbody>';

		tableHTML += '</table>';
		document.getElementById('general_table').innerHTML = tableHTML;
	}

	async function fill_tank_table(model_data) {
		let tableHTML = '<table>'
		
		var tableHeaders = ['parameter', 'value'];
		tableHTML += '<thead><tr>';
		tableHeaders.forEach(header => {
		    tableHTML += `<th>${header}</th>`;
		  });
		tableHTML += '</tr></thead>';

		var perf;
		if("integrated_system" in model_data)
		{
			wh = model_data["integrated_system"];
			perf = wh["performance"];
		}
		else
			perf = model_data["central_system"]			

		let tank_perf = perf["tank"]["performance"]

		tableEntries = ['volume', 'ua', 'fittings_ua', 'bottom_fraction_of_tank_mixing_on_draw'];
		tableHTML += '<tbody>';
		tableEntries.forEach(entry => {	
			if (entry in tank_perf)
				tableHTML += '<tr> <th>' + entry + '</th> <td>' + tank_perf[entry] + '</td> </tr>';
		});
		tableHTML += '</tbody>';

		tableHTML += '</table>';
		document.getElementById('tank_table').innerHTML = tableHTML;
	}

	async function fill_heatsource_table(heat_source_config) {
		let tableHTML = '<table>'
			
		var tableHeaders = ['parameter', 'value'];
		tableHTML += '<thead><tr>';
		tableHeaders.forEach(header => {
		    tableHTML += `<th>${header}</th>`;
		  });
		tableHTML += '</tr></thead>';

		const tableEntries = ['heat_source_type'];
		tableHTML += '<tbody>';
		tableEntries.forEach(entry => {	
			if (entry in heat_source_config)
				tableHTML += '<tr> <th>' + entry + '</th> <td>' + heat_source_config[entry] + '</td> </tr>';
		});

		let heat_source = heat_source_config["heat_source"];
		let heat_source_perf = heat_source["performance"]

		tableHTML += '</tbody>';

		tableHTML += '</table>';
		document.getElementById('heat_source_table').innerHTML = tableHTML;
	}

	async function fill_properties_table() {
		prefs = await read_json_file("./prefs.json")

		const ref_model_filepath = "../../../test/models_json/" + prefs['model_id'] + ".json";
		var model_cache = await read_json_file("./model_cache.json");
		if (!(prefs['model_id'] in model_cache))
		{
				model_cache[prefs['model_id']] = prefs["build_dir"] + "/gui/" + prefs['model_id'] + ".json"
				await copy_json_file(ref_model_filepath, model_cache[prefs['model_id']]);
				await write_json_file("./model_cache.json", model_cache);
		}
		model_filepath = 	model_cache[prefs['model_id']];
		model_data = await read_json_file(model_filepath);

		await fill_general_table(model_data);
		await fill_tank_table(model_data);

		var performance;
		if("integrated_system" in model_data)
		{
			wh = model_data["integrated_system"];
			performance = wh["performance"];
		}
		else
			performance = model_data["central_system"]	;		
		performance["heat_source_configurations"].forEach(hsc => {	
			if (hsc["id"] == prefs["heat_source_id"])
			{
					fill_heatsource_table(hsc);
			}
		}
		);
	}

async function init_all() {
	await init_websocket();
	await init_elements();
	await fill_fit_table();
	await fill_properties_table();
}