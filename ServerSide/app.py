import json
from flask import Flask, request, jsonify

app = Flask(__name__)


with open("win10-19043-exports.json", "r") as file:
    data = json.load(file)

@app.route('/check', methods=['GET'])
def check_query():
    query = request.args.get('query')
    if not query:
        return jsonify({"success": False, "message": "No query provided"}), 400

    for entry in data:
        if query.lower() in [entry["dllname"].lower(), entry["exportname"].lower()]:
            return jsonify({"success": True, "result": True})
    
    return jsonify({"success": True, "result": False})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)